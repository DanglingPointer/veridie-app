package com.vasilyev.veridie.interop;

public interface Command
{
    // command IDs, must be in sync with core/commands.hpp but bitshifted by 8 to the right
    int ID_START_LISTENING = 100;
    int ID_START_DISCOVERY = 101;
    int ID_STOP_LISTENING = 102;
    int ID_STOP_DISCOVERY = 103;
    int ID_CLOSE_CONNECTION = 104;
    int ID_ENABLE_BLUETOOTH = 105;
    int ID_NEGOTIATION_START = 106;
    int ID_NEGOTIATION_STOP = 107;
    int ID_SEND_MESSAGE = 108;
    int ID_SHOW_AND_EXIT = 109;
    int ID_SHOW_TOAST = 110;
    int ID_SHOW_NOTIFICATION = 111;
    int ID_SHOW_REQUEST = 112;
    int ID_SHOW_RESPONSE = 113;
    int ID_RESET_GAME = 114;

    // result codes, must be in sync with core/commands.hpp
    long ERROR_NO_ERROR = 0;
    long ERROR_INVALID_STATE = 0xffffffffffffffffL;
    long ERROR_BLUETOOTH_OFF = 2;
    long ERROR_LISTEN_FAILED = 3;
    long ERROR_CONNECTION_NOT_FOUND = 4;
    long ERROR_NO_BT_ADAPTER = 5;
    long ERROR_USER_DECLINED = 6;
    long ERROR_SOCKET_ERROR = 7;

    int getId();
    int getNativeId();
    String[] getArgs();
    void respond(long result);
}
