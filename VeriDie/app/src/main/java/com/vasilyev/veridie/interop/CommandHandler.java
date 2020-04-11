package com.vasilyev.veridie.interop;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;


public abstract class CommandHandler extends Handler
{
    public CommandHandler(Looper looper) {
        super(looper);
    }

    @Override
    public final void handleMessage(Message msg) {
        Command cmd = (Command)msg.obj;
        onCommandReceived(cmd);
    }

    protected abstract void onCommandReceived(Command cmd);
}
