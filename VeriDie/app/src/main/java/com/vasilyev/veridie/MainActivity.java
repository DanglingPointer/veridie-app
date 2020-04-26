package com.vasilyev.veridie;

import android.bluetooth.BluetoothDevice;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.vasilyev.veridie.bluetooth.BluetoothService;
import com.vasilyev.veridie.fragments.IdleFragment;
import com.vasilyev.veridie.interop.Bridge;
import com.vasilyev.veridie.interop.Command;
import com.vasilyev.veridie.interop.CommandHandler;
import com.vasilyev.veridie.interop.Event;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity implements BluetoothService.Callbacks, IdleFragment.Callbacks
{
    static {
        System.loadLibrary("veridie");
    }

    private static final String TAG = "MainActivity";

    private final ServiceConnection m_bluetoothConnection = new ServiceConnection()
    {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service)
        {
            m_bluetooth = ((BluetoothService.BluetoothBinder)service).getService();

            final Handler mainHandler = new Handler(Looper.getMainLooper());
            m_bluetooth.setCallbacks(MainActivity.this, mainHandler::post);
        }

        @Override
        public void onServiceDisconnected(ComponentName name)
        {
            m_bluetooth = null;
        }
    };
    private final Map<Integer, Command.Callback> m_handlers = new HashMap<Integer, Command.Callback>()
    {{
        put(Command.ID_NEGOTIATION_START, MainActivity.this::negotiationStart);
        put(Command.ID_NEGOTIATION_STOP, MainActivity.this::negotiationStop);
        put(Command.ID_SHOW_AND_EXIT, MainActivity.this::showAndExit);
        put(Command.ID_SHOW_TOAST, MainActivity.this::showToast);
        put(Command.ID_SHOW_NOTIFICATION, MainActivity.this::showNotification);
        put(Command.ID_SHOW_REQUEST, MainActivity.this::showRequest);
        put(Command.ID_SHOW_RESPONSE, MainActivity.this::showResponse);
        put(Command.ID_RESET_GAME, MainActivity.this::resetGame);
    }};
    private final CommandHandler m_cmdHandler = new CommandHandler(Looper.getMainLooper())
    {
        @Override
        protected void onCommandReceived(Command cmd)
        {
            try {
                m_handlers.get(cmd.getId()).onCommand(cmd);
            } catch (Exception e) {
                e.printStackTrace();
                cmd.respond(Command.ERROR_INVALID_STATE);
            }
        }
    };
    private final List<Command> m_pendingCmds = new ArrayList<>();

    private BluetoothService m_bluetooth;

    // ---------------Lifecycle-methods-------------------------------------------------------------

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Bridge.setUiCmdHandler(m_cmdHandler);

        Intent startBtIntent = new Intent(this, BluetoothService.class);
        startService(startBtIntent);
        Intent bindBtIntent = new Intent(this, BluetoothService.class);
        bindService(bindBtIntent, m_bluetoothConnection, BIND_AUTO_CREATE | BIND_IMPORTANT);

        getSupportFragmentManager().beginTransaction()
                .add(R.id.container, IdleFragment.newInstance())
                .commit();
    }

    @Override
    protected void onDestroy()
    {
        unbindService(m_bluetoothConnection);
        Bridge.setUiCmdHandler(null);
        super.onDestroy();
    }

    // ---------------Command-callbacks-------------------------------------------------------------

    private void negotiationStart(Command cmd)
    {

    }

    private void negotiationStop(Command cmd)
    {

    }

    private void showAndExit(Command cmd)
    {
        Toast.makeText(this, cmd.getArgs()[0], Toast.LENGTH_LONG).show(); // temp
    }

    private void showToast(Command cmd)
    {

    }

    private void showNotification(Command cmd)
    {

    }

    private void showRequest(Command cmd)
    {

    }

    private void showResponse(Command cmd)
    {

    }

    private void resetGame(Command cmd)
    {

    }

    // ---------------BluetoothService-callbacks----------------------------------------------------

    @Override
    public void deviceDiscovered(BluetoothDevice device)
    {

    }

    @Override
    public void deviceConnected(BluetoothDevice device)
    {

    }


    // ---------------IdleFragment-callbacks--------------------------------------------------------

    @Override
    public void onStartConnectingButtonPressed()
    {
        Bridge.send(Event.NEW_GAME_REQUESTED);
        //TODO: change fragment
    }
}
