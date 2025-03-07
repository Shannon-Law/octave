/*
    This source file was part of Konsole, a terminal emulator.

    Copyright (C) 2007, 2013 by Robert Knight <robertknight@gmail.com>

    Rewritten for QT4 by e_k <e_k at users.sourceforge.net>, Copyright (C)2008

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "unix/KeyboardTranslator.h"

// System
#include <ctype.h>
#include <stdio.h>

// Qt
#include <QtCore>
#include <QtGui>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

// FIXME: We should not have a special case for Mac here.  Instead, we
// should be loading .keytab files at run time, and ideally, allowing
// individual keys to be redefined from some preferences menu.

//and this is default now translator - default.keytab from original Konsole
const char* KeyboardTranslatorManager::defaultTranslatorText =
#if defined (Q_OS_MAC)
#include "ExtendedDefaultTranslatorMac.h"
#else
#include "ExtendedDefaultTranslator.h"
#endif
;

KeyboardTranslatorManager::KeyboardTranslatorManager()
    : _haveLoadedAll(false)
{
}
KeyboardTranslatorManager::~KeyboardTranslatorManager()
{
    qDeleteAll(_translators.values());
}
QString KeyboardTranslatorManager::findTranslatorPath(const QString& name)
{
    return QString("kb-layouts/" + name + ".keytab");
}
void KeyboardTranslatorManager::findTranslators()
{
    QDir dir("kb-layouts/");
    QStringList filters;
    filters << "*.keytab";
    dir.setNameFilters(filters);
    QStringList list = dir.entryList(filters); //(".keytab"); // = KGlobal::dirs()->findAllResources("data",
    //                                 "konsole/*.keytab",
    //                                 KStandardDirs::NoDuplicates);
    list = dir.entryList(filters);
    // add the name of each translator to the list and associated
    // the name with a null pointer to indicate that the translator
    // has not yet been loaded from disk
    QStringListIterator listIter(list);
    while (listIter.hasNext())
    {
        QString translatorPath = listIter.next();

        QString name = QFileInfo(translatorPath).baseName();

        if ( !_translators.contains(name) ) {
            _translators.insert(name,nullptr);
	}
    }
    _haveLoadedAll = true;
}

const KeyboardTranslator* KeyboardTranslatorManager::findTranslator(const QString& name)
{
    if ( name.isEmpty() )
        return defaultTranslator();

    //here was smth wrong in original Konsole source
    findTranslators();

    if ( _translators.contains(name) && _translators[name] != nullptr ) {
        return _translators[name];
    }

    KeyboardTranslator* translator = loadTranslator(name);

    if ( translator != nullptr )
        _translators[name] = translator;
    else if ( !name.isEmpty() )
        qWarning() << "Unable to load translator" << name;

    return translator;
}

bool KeyboardTranslatorManager::saveTranslator(const KeyboardTranslator* translator)
{
    const QString path = ".keytab";// = KGlobal::dirs()->saveLocation("data","konsole/")+translator->name()
    //           +".keytab";

    qDebug() << "Saving translator to" << path;

    QFile destination(path);

    if (!destination.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Unable to save keyboard translation:"
                   << destination.errorString();

        return false;
    }

    {
        KeyboardTranslatorWriter writer(&destination);
        writer.writeHeader(translator->description());

        QListIterator<KeyboardTranslator::Entry> iter(translator->entries());
        while ( iter.hasNext() )
            writer.writeEntry(iter.next());
    }

    destination.close();

    return true;
}

KeyboardTranslator* KeyboardTranslatorManager::loadTranslator(const QString& name)
{
    const QString& path = findTranslatorPath(name);

    QFile source(path);

    if (name.isEmpty() || !source.open(QIODevice::ReadOnly | QIODevice::Text))
        return nullptr;

    return loadTranslator(&source,name);
}

const KeyboardTranslator* KeyboardTranslatorManager::defaultTranslator()
{
    QBuffer textBuffer;
    textBuffer.setData(defaultTranslatorText,strlen(defaultTranslatorText));

    if (!textBuffer.open(QIODevice::ReadOnly))
        return nullptr;

    return loadTranslator(&textBuffer,"fallback");
}

KeyboardTranslator* KeyboardTranslatorManager::loadTranslator(QIODevice* source,const QString& name)
{
    KeyboardTranslator* translator = new KeyboardTranslator(name);
    KeyboardTranslatorReader reader(source);
    translator->setDescription( reader.description() );

    while ( reader.hasNextEntry() ) {
        translator->addEntry(reader.nextEntry());
    }	

    source->close();

    if ( !reader.parseError() )
    {
        return translator;
    }
    else
    {
        delete translator;
        return nullptr;
    }
}

KeyboardTranslatorWriter::KeyboardTranslatorWriter(QIODevice* destination)
    : _destination(destination)
{
    Q_ASSERT( destination && destination->isWritable() );

    _writer = new QTextStream(_destination);
}
KeyboardTranslatorWriter::~KeyboardTranslatorWriter()
{
    delete _writer;
}
void KeyboardTranslatorWriter::writeHeader( const QString& description )
{
    *_writer << "keyboard \"" << description << '\"' << '\n';
}
void KeyboardTranslatorWriter::writeEntry( const KeyboardTranslator::Entry& entry )
{
    QString result;

    if ( entry.command() != KeyboardTranslator::NoCommand )
        result = entry.resultToString();
    else
        result = '\"' + entry.resultToString() + '\"';

    *_writer << "key " << entry.conditionToString() << " : " << result << '\n';
}


// each line of the keyboard translation file is one of:
//
// - keyboard "name"
// - key KeySequence : "characters"
// - key KeySequence : CommandName
//
// KeySequence begins with the name of the key ( taken from the Qt::Key enum )
// and is followed by the keyboard modifiers and state flags ( with + or - in front
// of each modifier or flag to indicate whether it is required ).  All keyboard modifiers
// and flags are optional, if a particular modifier or state is not specified it is
// assumed not to be a part of the sequence.  The key sequence may contain whitespace
//
// eg:  "key Up+Shift : scrollLineUp"
//      "key Next-Shift : "\E[6~"
//
// (lines containing only whitespace are ignored, parseLine assumes that comments have
// already been removed)
//

KeyboardTranslatorReader::KeyboardTranslatorReader( QIODevice* source )
    : _source(source)
    , _hasNext(false)
{
    // read input until we find the description
    while ( _description.isEmpty() && !source->atEnd() )
    {
        const QList<Token>& tokens = tokenize( QString(source->readLine()) );

        if ( !tokens.isEmpty() && tokens.first().type == Token::TitleKeyword )
        {
            _description = (tokens[1].text.toUtf8());
        }
    }

    readNext();
}
void KeyboardTranslatorReader::readNext()
{
    // find next entry
    while ( !_source->atEnd() )
    {
        const QList<Token>& tokens = tokenize( QString(_source->readLine()) );
        if ( !tokens.isEmpty() && tokens.first().type == Token::KeyKeyword )
        {
            KeyboardTranslator::States flags = KeyboardTranslator::NoState;
            KeyboardTranslator::States flagMask = KeyboardTranslator::NoState;
            Qt::KeyboardModifiers modifiers = Qt::NoModifier;
            Qt::KeyboardModifiers modifierMask = Qt::NoModifier;

            int keyCode = Qt::Key_unknown;

            decodeSequence(tokens[1].text.toLower(),
                           keyCode,
                           modifiers,
                           modifierMask,
                           flags,
                           flagMask);

            KeyboardTranslator::Command command = KeyboardTranslator::NoCommand;
            QByteArray text;

            // get text or command
            if ( tokens[2].type == Token::OutputText )
            {
                text = tokens[2].text.toLocal8Bit();
            }
            else if ( tokens[2].type == Token::Command )
            {
                // identify command
                if (!parseAsCommand(tokens[2].text,command))
                    qWarning() << "Command" << tokens[2].text << "not understood.";
            }

            KeyboardTranslator::Entry newEntry;
            newEntry.setKeyCode( keyCode );
            newEntry.setState( flags );
            newEntry.setStateMask( flagMask );
            newEntry.setModifiers( modifiers );
            newEntry.setModifierMask( modifierMask );
            newEntry.setText( text );
            newEntry.setCommand( command );

            _nextEntry = newEntry;

            _hasNext = true;

            return;
        }
    }

    _hasNext = false;
}

bool KeyboardTranslatorReader::parseAsCommand(const QString& text,KeyboardTranslator::Command& command)
{
    if ( text.compare("erase",Qt::CaseInsensitive) == 0 )
        command = KeyboardTranslator::EraseCommand;
    else if ( text.compare("scrollpageup",Qt::CaseInsensitive) == 0 )
        command = KeyboardTranslator::ScrollPageUpCommand;
    else if ( text.compare("scrollpagedown",Qt::CaseInsensitive) == 0 )
        command = KeyboardTranslator::ScrollPageDownCommand;
    else if ( text.compare("scrolllineup",Qt::CaseInsensitive) == 0 )
        command = KeyboardTranslator::ScrollLineUpCommand;
    else if ( text.compare("scrolllinedown",Qt::CaseInsensitive) == 0 )
        command = KeyboardTranslator::ScrollLineDownCommand;
    else if ( text.compare("scrolllock",Qt::CaseInsensitive) == 0 )
        command = KeyboardTranslator::ScrollLockCommand;
    else
    	return false;

    return true;
}

bool KeyboardTranslatorReader::decodeSequence(const QString& text,
                                              int& keyCode,
                                              Qt::KeyboardModifiers& modifiers,
                                              Qt::KeyboardModifiers& modifierMask,
                                              KeyboardTranslator::States& flags,
                                              KeyboardTranslator::States& flagMask)
{
    bool isWanted = true;
    bool endOfItem = false;
    QString buffer;

    Qt::KeyboardModifiers tempModifiers = modifiers;
    Qt::KeyboardModifiers tempModifierMask = modifierMask;
    KeyboardTranslator::States tempFlags = flags;
    KeyboardTranslator::States tempFlagMask = flagMask;

    for ( int i = 0 ; i < text.count() ; i++ )
    {
        const QChar& ch = text[i];
        bool isLastLetter = ( i == text.count()-1 );

        endOfItem = true;
        if ( ch.isLetterOrNumber() )
        {
            endOfItem = false;
            buffer.append(ch);
        }

        if ( (endOfItem || isLastLetter) && !buffer.isEmpty() )
        {
            Qt::KeyboardModifier itemModifier = Qt::NoModifier;
            int itemKeyCode = 0;
            KeyboardTranslator::State itemFlag = KeyboardTranslator::NoState;

            if ( parseAsModifier(buffer,itemModifier) )
            {
                tempModifierMask |= itemModifier;

                if ( isWanted )
                    tempModifiers |= itemModifier;
            }
            else if ( parseAsStateFlag(buffer,itemFlag) )
            {
                tempFlagMask |= itemFlag;

                if ( isWanted )
                    tempFlags |= itemFlag;
            }
            else if ( parseAsKeyCode(buffer,itemKeyCode) )
                keyCode = itemKeyCode;
            else
                qDebug() << "Unable to parse key binding item:" << buffer;

            buffer.clear();
        }

        // check if this is a wanted / not-wanted flag and update the
        // state ready for the next item
        if ( ch == '+' )
            isWanted = true;
        else if ( ch == '-' )
            isWanted = false;
    }

    modifiers = tempModifiers;
    modifierMask = tempModifierMask;
    flags = tempFlags;
    flagMask = tempFlagMask;

    return true;
}

bool KeyboardTranslatorReader::parseAsModifier(const QString& item , Qt::KeyboardModifier& modifier)
{
    if ( item == "shift" )
        modifier = Qt::ShiftModifier;
    else if ( item == "ctrl" || item == "control" )
        modifier = Qt::ControlModifier;
    else if ( item == "alt" )
        modifier = Qt::AltModifier;
    else if ( item == "meta" )
        modifier = Qt::MetaModifier;
    else if ( item == "keypad" )
        modifier = Qt::KeypadModifier;
    else
        return false;

    return true;
}
bool KeyboardTranslatorReader::parseAsStateFlag(const QString& item , KeyboardTranslator::State& flag)
{
    if ( item == "appcukeys" )
        flag = KeyboardTranslator::CursorKeysState;
    else if ( item == "ansi" )
        flag = KeyboardTranslator::AnsiState;
    else if ( item == "newline" )
        flag = KeyboardTranslator::NewLineState;
    else if ( item == "appscreen" )
        flag = KeyboardTranslator::AlternateScreenState;
    else if ( item == "anymod" )
        flag = KeyboardTranslator::AnyModifierState;
    else
        return false;

    return true;
}
bool KeyboardTranslatorReader::parseAsKeyCode(const QString& item , int& keyCode)
{
    QKeySequence sequence = QKeySequence::fromString(item);
    if ( !sequence.isEmpty() )
    {
        keyCode = sequence[0];

        if ( sequence.count() > 1 )
        {
            qDebug() << "Unhandled key codes in sequence: " << item;
        }
    }
    // additional cases implemented for backwards compatibility with KDE 3
    else if ( item == "prior" )
        keyCode = Qt::Key_PageUp;
    else if ( item == "next" )
        keyCode = Qt::Key_PageDown;
    else
        return false;

    return true;
}

QString KeyboardTranslatorReader::description() const
{
    return _description;
}
bool KeyboardTranslatorReader::hasNextEntry()
{
    return _hasNext;
}
KeyboardTranslator::Entry KeyboardTranslatorReader::createEntry( const QString& condition ,
                                                                 const QString& result )
{
    QString entryString("keyboard \"temporary\"\nkey ");
    entryString.append(condition);
    entryString.append(" : ");

    // if 'result' is the name of a command then the entry result will be that command,
    // otherwise the result will be treated as a string to echo when the key sequence
    // specified by 'condition' is pressed
    KeyboardTranslator::Command command;
    if (parseAsCommand(result,command))
    	entryString.append(result);
    else
        entryString.append('\"' + result + '\"');

    QByteArray array = entryString.toUtf8();

    KeyboardTranslator::Entry entry;

    QBuffer buffer(&array);
    buffer.open(QIODevice::ReadOnly);
    KeyboardTranslatorReader reader(&buffer);

    if ( reader.hasNextEntry() )
        entry = reader.nextEntry();

    return entry;
}

KeyboardTranslator::Entry KeyboardTranslatorReader::nextEntry()
{
    Q_ASSERT( _hasNext );


    KeyboardTranslator::Entry entry = _nextEntry;

    readNext();

    return entry;
}
bool KeyboardTranslatorReader::parseError()
{
    return false;
}
QList<KeyboardTranslatorReader::Token>
KeyboardTranslatorReader::tokenize (const QString& line)
{
    QString text = line.simplified();

    // comment line: # comment
    static QRegularExpression comment {"\\#.*"};
    // title line: keyboard "title"
    static QRegularExpression title {"keyboard\\s+\"(.*)\""};
    // key line: key KeySequence : "output"
    // key line: key KeySequence : command
    static QRegularExpression key {"key\\s+([\\w\\+\\s\\-]+)\\s*:\\s*(\"(.*)\"|\\w+)"};

    QList<Token> list;

    if ( text.isEmpty() || comment.match (text).hasMatch () )
    {
        return list;
    }

    QRegularExpressionMatch match;
    if ((match = title.match (text)).hasMatch ())
    {
        Token titleToken = { Token::TitleKeyword , QString() };
        Token textToken = { Token::TitleText , match.captured (1) };

        list << titleToken << textToken;
    }
    else if  ((match = key.match (text)).hasMatch ())
    {
        Token keyToken = { Token::KeyKeyword , QString() };
        Token sequenceToken = { Token::KeySequence,
                                match.captured (1).remove (' ') };

        list << keyToken << sequenceToken;

        if ( match.captured (3).isEmpty () )
        {
            // capturedTexts()[2] is a command
            Token commandToken = { Token::Command , match.captured (2) };
            list << commandToken;
        }
        else
        {
            // capturedTexts()[3] is the output string
            Token outputToken = { Token::OutputText , match.captured (3) };
            list << outputToken;
        }
    }
    else
    {
        qWarning() << "Line in keyboard translator file could not be understood:" << text;
    }

    return list;
}

QList<QString> KeyboardTranslatorManager::allTranslators()
{
    if ( !_haveLoadedAll )
    {
        findTranslators();
    }

    return _translators.keys();
}

KeyboardTranslator::Entry::Entry()
    : _keyCode(0)
    , _modifiers(Qt::NoModifier)
    , _modifierMask(Qt::NoModifier)
    , _state(NoState)
    , _stateMask(NoState)
    , _command(NoCommand)
{
}

bool KeyboardTranslator::Entry::operator==(const Entry& rhs) const
{
    return _keyCode == rhs._keyCode &&
            _modifiers == rhs._modifiers &&
            _modifierMask == rhs._modifierMask &&
            _state == rhs._state &&
            _stateMask == rhs._stateMask &&
            _command == rhs._command &&
            _text == rhs._text;
}

bool KeyboardTranslator::Entry::matches(int keyCode ,
                                        Qt::KeyboardModifiers modifiers,
                                        States state) const
{
    if ( _keyCode != keyCode )
        return false;

    if ( (modifiers & _modifierMask) != (_modifiers & _modifierMask) )
        return false;

    // if modifiers is non-zero, the 'any modifier' state is implicit
    if ( modifiers != 0 )
        state |= AnyModifierState;

    if ( (state & _stateMask) != (_state & _stateMask) )
        return false;

    // special handling for the 'Any Modifier' state, which checks for the presence of
    // any or no modifiers.  In this context, the 'keypad' modifier does not count.
    bool anyModifiersSet = modifiers != 0 && modifiers != Qt::KeypadModifier;
    if ( _stateMask & KeyboardTranslator::AnyModifierState )
    {
        // test fails if any modifier is required but none are set
        if ( (_state & KeyboardTranslator::AnyModifierState) && !anyModifiersSet )
            return false;

        // test fails if no modifier is allowed but one or more are set
        if ( !(_state & KeyboardTranslator::AnyModifierState) && anyModifiersSet )
            return false;
    }

    return true;
}
QByteArray KeyboardTranslator::Entry::escapedText(bool expandWildCards,Qt::KeyboardModifiers modifiers) const
{
    QByteArray result(text(expandWildCards,modifiers));

    for ( int i = 0 ; i < result.count() ; i++ )
    {
        char ch = result[i];
        char replacement = 0;

        switch ( ch )
        {
        case 27 : replacement = 'E'; break;
        case 8  : replacement = 'b'; break;
        case 12 : replacement = 'f'; break;
        case 9  : replacement = 't'; break;
        case 13 : replacement = 'r'; break;
        case 10 : replacement = 'n'; break;
        default:
            // any character which is not printable is replaced by an equivalent
            // \xhh escape sequence (where 'hh' are the corresponding hex digits)
            if ( !QChar(ch).isPrint() )
                replacement = 'x';
        }

        if ( replacement == 'x' )
        {
          result.replace(i,1,"\\x"+QByteArray::number(QByteArray(1,ch).toInt(nullptr, 16)));
        } else if ( replacement != 0 )
        {
            result.remove(i,1);
            result.insert(i,'\\');
            result.insert(i+1,replacement);
        }
    }

    return result;
}
QByteArray KeyboardTranslator::Entry::unescape(const QByteArray& input) const
{
    QByteArray result(input);

    for ( int i = 0 ; i < result.count()-1 ; i++ )
    {

        char ch = result[i];
        if ( ch == '\\' )
        {
            char replacement[2] = {0,0};
            int charsToRemove = 2;
            bool escapedChar = true;

            switch ( result[i+1] )
            {
            case 'E' : replacement[0] = 27; break;
            case 'b' : replacement[0] = 8 ; break;
            case 'f' : replacement[0] = 12; break;
            case 't' : replacement[0] = 9 ; break;
            case 'r' : replacement[0] = 13; break;
            case 'n' : replacement[0] = 10; break;
            case 'x' :
            {
                // format is \xh or \xhh where 'h' is a hexadecimal
                // digit from 0-9 or A-F which should be replaced
                // with the corresponding character value
                char hexDigits[3] = {0};

                if ( (i < result.count()-2) && isxdigit(result[i+2]) )
                    hexDigits[0] = result[i+2];
                if ( (i < result.count()-3) && isxdigit(result[i+3]) )
                    hexDigits[1] = result[i+3];

                int charValue = 0;
                sscanf(hexDigits,"%x",&charValue);

                replacement[0] = (char)charValue;

                charsToRemove = 2 + strlen(hexDigits);
            }
            break;
            default:
                escapedChar = false;
            }

            if ( escapedChar )
                result.replace(i,charsToRemove,replacement);
        }
    }

    return result;
}

void KeyboardTranslator::Entry::insertModifier( QString& item , int modifier ) const
{
    if ( !(modifier & _modifierMask) )
        return;

    if ( modifier & _modifiers )
        item += '+';
    else
        item += '-';

    if ( modifier == Qt::ShiftModifier )
        item += "Shift";
    else if ( modifier == Qt::ControlModifier )
        item += "Ctrl";
    else if ( modifier == Qt::AltModifier )
        item += "Alt";
    else if ( modifier == Qt::MetaModifier )
        item += "Meta";
    else if ( modifier == Qt::KeypadModifier )
        item += "KeyPad";
}
void KeyboardTranslator::Entry::insertState( QString& item , int state ) const
{
    if ( !(state & _stateMask) )
        return;

    if ( state & _state )
        item += '+' ;
    else
        item += '-' ;

    if ( state == KeyboardTranslator::AlternateScreenState )
        item += "AppScreen";
    else if ( state == KeyboardTranslator::NewLineState )
        item += "NewLine";
    else if ( state == KeyboardTranslator::AnsiState )
        item += "Ansi";
    else if ( state == KeyboardTranslator::CursorKeysState )
        item += "AppCuKeys";
    else if ( state == KeyboardTranslator::AnyModifierState )
        item += "AnyMod";
}
QString KeyboardTranslator::Entry::resultToString(bool expandWildCards,Qt::KeyboardModifiers modifiers) const
{
    if ( !_text.isEmpty() )
        return escapedText(expandWildCards,modifiers);
    else if ( _command == EraseCommand )
        return "Erase";
    else if ( _command == ScrollPageUpCommand )
        return "ScrollPageUp";
    else if ( _command == ScrollPageDownCommand )
        return "ScrollPageDown";
    else if ( _command == ScrollLineUpCommand )
        return "ScrollLineUp";
    else if ( _command == ScrollLineDownCommand )
        return "ScrollLineDown";
    else if ( _command == ScrollLockCommand )
        return "ScrollLock";

    return QString();
}
QString KeyboardTranslator::Entry::conditionToString() const
{
    QString result = QKeySequence(_keyCode).toString();

    // add modifiers
    insertModifier( result , Qt::ShiftModifier );
    insertModifier( result , Qt::ControlModifier );
    insertModifier( result , Qt::AltModifier );
    insertModifier( result , Qt::MetaModifier );

    // add states
    insertState( result , KeyboardTranslator::AlternateScreenState );
    insertState( result , KeyboardTranslator::NewLineState );
    insertState( result , KeyboardTranslator::AnsiState );
    insertState( result , KeyboardTranslator::CursorKeysState );
    insertState( result , KeyboardTranslator::AnyModifierState );

    return result;
}

KeyboardTranslator::KeyboardTranslator(const QString& name)
    : _name(name)
{
}

void KeyboardTranslator::setDescription(const QString& description)
{
    _description = description;
}
QString KeyboardTranslator::description() const
{
    return _description;
}
void KeyboardTranslator::setName(const QString& name)
{
    _name = name;
}
QString KeyboardTranslator::name() const
{
    return _name;
}

QList<KeyboardTranslator::Entry> KeyboardTranslator::entries() const
{
    return _entries.values();
}

void KeyboardTranslator::addEntry(const Entry& entry)
{
    const int keyCode = entry.keyCode();
    _entries.insert(keyCode,entry);
}
void KeyboardTranslator::replaceEntry(const Entry& existing , const Entry& replacement)
{
    if ( !existing.isNull() )
        _entries.remove(existing.keyCode());
    _entries.insert(replacement.keyCode(),replacement);
}
void KeyboardTranslator::removeEntry(const Entry& entry)
{
    _entries.remove(entry.keyCode());
}
KeyboardTranslator::Entry KeyboardTranslator::findEntry(int keyCode, Qt::KeyboardModifiers modifiers, States state) const
{
    if ( _entries.contains(keyCode) )
    {
        QList<Entry> entriesForKey = _entries.values(keyCode);

        QListIterator<Entry> iter(entriesForKey);

        while (iter.hasNext())
        {
            const Entry& next = iter.next();
            if ( next.matches(keyCode,modifiers,state) )
                return next;
        }

        return Entry(); // entry not found
    }
    else
    {
        return Entry();
    }

}
void KeyboardTranslatorManager::addTranslator(KeyboardTranslator* translator)
{
    _translators.insert(translator->name(),translator);

    if ( !saveTranslator(translator) )
        qWarning() << "Unable to save translator" << translator->name()
                   << "to disk.";
}
bool KeyboardTranslatorManager::deleteTranslator(const QString& name)
{
    Q_ASSERT( _translators.contains(name) );

    // locate and delete
    QString path = findTranslatorPath(name);
    if ( QFile::remove(path) )
    {
        _translators.remove(name);
        return true;
    }
    else
    {
        qWarning() << "Failed to remove translator - " << path;
        return false;
    }
}
K_GLOBAL_STATIC( KeyboardTranslatorManager , theKeyboardTranslatorManager )
KeyboardTranslatorManager* KeyboardTranslatorManager::instance()
{
    return theKeyboardTranslatorManager;
}
