/*
 *
 *
 * Copyright 2013 Oscar Cerna
 *
 * This file is part of CasterSoundboard. An application for playing hot-keyed sounds.
 * For more information, please visit http://sourceforge.net/projects/castersb.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU LESSER GENERAL PUBLIC LICENSE for more details.
 *
 * You should have received a copy of the GNU LESSER GENERAL PUBLIC LICENSE
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "CasterBoard.h"
#include "CasterPlayer.h"
#include "CasterPlayerState.h"
#include <QGridLayout>
#include <QFile>
#include <QDataStream>
#include <QString>
#include "libs/osc/composer/OscMessageComposer.h"

//Constructor=============================================
CasterBoard::CasterBoard(QWidget* parent) : QWidget(parent)
{
    QGridLayout *boardLayout = new QGridLayout(this);

    //Properties
    soundBoardName = new QString("No Name");
    profileFilePath = new QString("");

    // MAPs
    // int_to_player_key
    int_to_player_key = new QMap<int,QString>
    {
//  1366 x 768 Layout
        {0,"1"},{1,"2"},{2,"3"},{3,"4"},{4,"5"},{5,"6"},{6,"7"},{7,"8"},{8,"9"},{9,"0"},
        {10,"Q"},{11,"W"},{12,"E"},{13,"R"},{14,"T"},{15,"Y"},{16,"U"},{17,"I"},{18,"O"},{19,"P"},
        {20,"A"},{21,"S"},{22,"D"},{23,"F"},{24,"G"},{25,"H"},{26,"J"},{27,"K"},{28,"L"},{29,";"},
        {30,"<"},{31,"Z"},{32,"X"},{33,"C"},{34,"V"},{35,"B"},{36,"N"},{37,"M"},{38,","},{39,"."}
    };
    // keyboard_key_to_player_key
    keyboard_key_to_player_key = new QMap<int,QString>
    {
// 1366 x 768 Layout
        {Qt::Key_1,"1"},{Qt::Key_2,"2"},{Qt::Key_3,"3"},{Qt::Key_4,"4"},{Qt::Key_5,"5"},{Qt::Key_6,"6"},{Qt::Key_7,"7"},{Qt::Key_8,"8"},{Qt::Key_9,"9"},{Qt::Key_0,"0"},
        {Qt::Key_Q,"Q"},{Qt::Key_W,"W"},{Qt::Key_E,"E"},{Qt::Key_R,"R"},{Qt::Key_T,"T"},{Qt::Key_Y,"Y"},{Qt::Key_U,"U"},{Qt::Key_I,"I"},{Qt::Key_O,"O"},{Qt::Key_P,"P"},
        {Qt::Key_A,"A"},{Qt::Key_S,"S"},{Qt::Key_D,"D"},{Qt::Key_F,"F"},{Qt::Key_G,"G"},{Qt::Key_H,"H"},{Qt::Key_J,"J"},{Qt::Key_K,"K"},{Qt::Key_L,"L"},{Qt::Key_Semicolon,";"},
        {Qt::Key_Less,"<"},{Qt::Key_Z,"Z"},{Qt::Key_X,"X"},{Qt::Key_C,"C"},{Qt::Key_V,"V"},{Qt::Key_B,"B"},{Qt::Key_N,"N"},{Qt::Key_M,"M"},{Qt::Key_Comma,","},{Qt::Key_Period,"."}
    };
    // players
    players = new QMap<QString, CasterPlayerWidget*>();

    /* Create Players */
    int _board_column = 0; int _board_row = 0;
    for(int i=0; i < int_to_player_key->count(); i++){
        // Initialize player
        players->insert(int_to_player_key->value(i), new CasterPlayerWidget);
        players->value(int_to_player_key->value(i))->setHotKeyLetter(int_to_player_key->value(i));

        // Connect OSC Events
        connect(players->value(int_to_player_key->value(i)),SIGNAL(updateOSCClient(OscMessageComposer*)),this,SLOT(notifyApplicationAboutOSCMessage(OscMessageComposer*)));

        // Add to layout
        boardLayout->addWidget(players->value(int_to_player_key->value(i)), _board_row, _board_column);

        // Update Next Layout
        _board_column += 1;
// 1366 x 768 Layout
        if(_board_column > 9){
            _board_row += 1;
            _board_column = 0;
        }
    }
}
//PUBLIC
void CasterBoard::stopAllSounds()
{
    // Iterate through players
    foreach(QString _letter, players->keys()){
        players->value(_letter)->stopSound();
    }
}

void CasterBoard::setAllAudioDuckingStates(int state)
{
    // Iterate through players
    foreach(QString _letter, players->keys()){
        players->value(_letter)->setAudioDuckState(state);
    }
}

void CasterBoard::notifyApplicationAboutOSCMessage(OscMessageComposer* message){
    if(this->isCurrentBoard)
        emit this->_updateOSCClient(message);
}

void CasterBoard::syncWithOSCClient()
{
    // Update Tab Name
    OscMessageComposer* msg = writeOSCMessage("/glo/m/label/tab_name", *soundBoardName);
    emit this->_updateOSCClient(msg);

    // Iterate through players
    foreach(QString _letter, players->keys()){
        players->value(_letter)->syncWithOSCClient();
    }
}

// PROTECTED
void CasterBoard::keyReleaseEvent(QKeyEvent *event)
{
    //Handles All Hot Key Behavior
    if(keyboard_key_to_player_key->contains(event->key())){
        if(players->contains(keyboard_key_to_player_key->value(event->key()))){
            // Play or stop sound based on player's play state
            if(players->value(keyboard_key_to_player_key->value(event->key()))->player->state() == QMediaPlayer::PlayingState)
                players->value(keyboard_key_to_player_key->value(event->key()))->stopSound();
            else if (players->value(keyboard_key_to_player_key->value(event->key()))->player->state() == QMediaPlayer::PausedState)
                players->value(keyboard_key_to_player_key->value(event->key()))->playSound();
            else if (players->value(keyboard_key_to_player_key->value(event->key()))->player->state() == QMediaPlayer::StoppedState)
                players->value(keyboard_key_to_player_key->value(event->key()))->playSound();
        }
    } else {
        // Delegates global hotkeys to MainWIndow
        emit globalHotKeyReleasedEvent(event);
    }

}

//Public Methods
void CasterBoard::reloadBoardFromPlayerStates()
{
    // Iterate through players
    foreach(QString _letter, players->keys()){
        players->value(_letter)->reloadFromPlayerState();
    }

    this->update();
}

//========================================================
//==========OSC Composer Methods=====
OscMessageComposer* CasterBoard::writeOSCMessage(QString address, int value){
    // Compose OSC Message
    OscMessageComposer* msg = new OscMessageComposer(address);
    msg->pushInt32((qint32)value);
    return msg;
}

OscMessageComposer* CasterBoard::writeOSCMessage(QString address, float value){
    // Compose OSC Message
    OscMessageComposer* msg = new OscMessageComposer(address);
    msg->pushFloat(value);
    return msg;
}

OscMessageComposer* CasterBoard::writeOSCMessage(QString address, QString value){
    // Compose OSC Message
    OscMessageComposer* msg = new OscMessageComposer(address);
    msg->pushString(value.toUtf8());
    return msg;
}

//================================================


//Operator Overloading
QDataStream &operator<<(QDataStream &ds, const CasterBoard &cb)
{
    /* Properties */
    ds << *cb.soundBoardName;
    /* All Player Properties */
    // Iterate through players
    foreach(QString _letter, cb.players->keys()){
        ds << *cb.players->value(_letter)->playerState;
    }
    return ds;
}


QDataStream &operator>>(QDataStream &ds, CasterBoard &cb)
{
    /* Properties */
    ds >> *cb.soundBoardName;
    /* All Player Properties */
    // Iterate through players
    foreach(QString _letter, cb.players->keys()){
        ds >> *cb.players->value(_letter)->playerState;
    }
    return ds;
}
