package com.vasilyev.veridie.interop;

import android.os.Message;
import android.text.TextUtils;
import android.util.Log;

public final class Bridge
{
    private static final String TAG = Bridge.class.getName();

    private static CommandHandler s_uiHandler;
    private static CommandHandler s_btHandler;
    public static void setCommandHandlers(CommandHandler uiHandler, CommandHandler btHandler) {
        s_uiHandler = uiHandler;
        s_btHandler = btHandler;
        bridgeReady();
    }

    public static void send(Event e) {
        Log.d(TAG, String.format("Sending event %d with args [%s]",
                e.getId(),
                (e.getArgs() != null) ? TextUtils.join(", ", e.getArgs()) : ""));
        sendEvent(e.getId(), e.getArgs());
    }


    // C++ --> Java
    private static void receiveUiCommand(int id, String[] args) {
        Log.d(TAG, String.format("Bridge received UI command, id = %d, args = [%s]",
                id,
                (args != null) ? TextUtils.join(", ", args) : ""));
        receiveCommand(id, args, s_uiHandler);
    }
    private static void receiveBtCommand(int id, String[] args) {
        Log.d(TAG, String.format("Bridge received BT command, id = %d, args = [%s]",
                id,
                (args != null) ? TextUtils.join(", ", args) : ""));
        receiveCommand(id, args, s_btHandler);
    }
    private static void receiveCommand(int id, String[] args, CommandHandler handler) {
        final int cmdId = id;
        final String[] cmdArgs = args;
        Command cmd = new Command() {
            @Override
            public int getId() {
                return cmdId >> 8;
            }
            @Override
            public int getNativeId() {
                return cmdId;
            }
            @Override
            public String[] getArgs() {
                return cmdArgs;
            }
            @Override
            public void respond(long result) {
                Log.d(TAG, "Responding to cmd " + cmdId + " with result: " + result);
                sendResponse(cmdId, result); // do not use getId() here!
            }
        };

        if (handler == null) {
            cmd.respond(Command.ERROR_INVALID_STATE);
            return;
        }
        Message msg = handler.obtainMessage(cmdId, cmd);
        msg.sendToTarget();
    }

    // Java --> C++
    private static native void bridgeReady();
    private static native void sendEvent(int eventId, String[] args);
    private static native void sendResponse(int cmdId, long result);
}
