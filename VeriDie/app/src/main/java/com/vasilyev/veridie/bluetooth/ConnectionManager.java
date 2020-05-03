package com.vasilyev.veridie.bluetooth;

import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Process;
import android.util.Log;

import com.vasilyev.veridie.interop.Bridge;
import com.vasilyev.veridie.interop.Command;
import com.vasilyev.veridie.interop.CommandHandler;
import com.vasilyev.veridie.interop.Event;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class ConnectionManager extends HandlerThread
{
    private static final String TAG = "ConnectionManager";
    private static final long INITIAL_READ_INTERVAL_MS = 500;

    private static class Connection
    {
        Connection(BluetoothSocket socket) throws IOException
        {
            this.socket = socket;
            this.instream = new DataInputStream(socket.getInputStream());
            this.outstream = new DataOutputStream(socket.getOutputStream());
        }

        final BluetoothSocket socket;
        final DataInputStream instream;
        final DataOutputStream outstream;
    }

    private final Map<String, Connection> m_connections = new HashMap<>(5);
    private final byte[] m_buffer = new byte[1024];

    private Handler m_internalHandler;

    public ConnectionManager()
    {
        super(TAG, Process.THREAD_PRIORITY_FOREGROUND);
    }

    @Override
    public synchronized void start()
    {
        super.start();
        m_internalHandler = new Handler(getLooper());
        m_internalHandler.postDelayed(this::readFromSockets, INITIAL_READ_INTERVAL_MS);
    }

    public void addConnection(BluetoothSocket socket) throws IOException
    {
        if (m_internalHandler == null)
            throw new IllegalStateException();

        final Connection conn = new Connection(socket);
        final String mac = socket.getRemoteDevice().getAddress();

        boolean success = m_internalHandler.post(() -> {
            m_connections.put(mac, conn);
            Bridge.send(Event.REMOTE_DEVICE_CONNECTED.withArgs(mac, conn.socket.getRemoteDevice().getName()));
        });
        if (!success)
            throw new IllegalStateException();
    }

    public void removeConnection(String name)
    {
        if (m_internalHandler == null)
            throw new IllegalStateException();

        boolean success = m_internalHandler.post(() -> {
            String mac = null;
            for (Connection c : m_connections.values()) {
                if (c.socket.getRemoteDevice().getName().equals(name)) {
                    mac = c.socket.getRemoteDevice().getAddress();
                    break;
                }
            }
            if (mac != null) {
                closeConnection(mac);
                Bridge.send(Event.REMOTE_DEVICE_DISCONNECTED.withArgs(mac, name));
            }
        });
        if (!success)
            throw new IllegalStateException();
    }

    public CommandHandler getCommandHandler()
    {
        Looper looper = getLooper();
        if (looper == null)
            throw new IllegalStateException();

        return new CommandHandler(looper)
        {
            @Override
            protected void onCommandReceived(Command cmd)
            {
                switch (cmd.getId()) {
                    case Command.ID_SEND_MESSAGE: {
                        long error = sendData(cmd.getArgs()[0], cmd.getArgs()[1]);
                        cmd.respond(error);
                        break;
                    }
                    case Command.ID_CLOSE_CONNECTION: {
                        long error = closeConnection(cmd.getArgs()[0]);
                        cmd.respond(error);
                        break;
                    }
                    default:
                        Log.wtf(TAG, "Inappropriate command received: id=" + cmd.getId());
                }
            }
        };
    }

    private void readFromSockets()
    {
        for (Map.Entry<String, Connection> entry : m_connections.entrySet()) {
            String mac = entry.getKey();
            DataInputStream stream = entry.getValue().instream;
            try {
                if (stream.available() < 2)
                    continue;
                final int length = stream.readShort();
                int read = 0;
                while(read < length) {
                    read += stream.read(m_buffer, read, length - read);
                }
                String message = new String(m_buffer, 0, read, StandardCharsets.UTF_8);
                Log.d(TAG, String.format("[ %s => ]: %s", mac, message));
                Bridge.send(Event.MESSAGE_RECEIVED.withArgs(message, mac, ""));
            }
            catch (IOException e) {
                Log.w(TAG, "Error while reading:" + e.getMessage());
                Bridge.send(Event.SOCKET_READ_FAILED.withArgs(mac, ""));
            }
        }
        m_internalHandler.postDelayed(this::readFromSockets, getReadInterval());
    }

    private long sendData(String receiverMac, String message)
    {
        Connection conn = m_connections.get(receiverMac);
        if (conn == null)
            return Command.ERROR_CONNECTION_NOT_FOUND;
        try {
            conn.outstream.writeShort(message.length());
            conn.outstream.write(message.getBytes(StandardCharsets.UTF_8));
            conn.outstream.flush();
            Log.d(TAG, String.format("[ %s <= ]: %s", receiverMac, message));
            return Command.ERROR_NO_ERROR;
        }
        catch (IOException e) {
            Log.w(TAG, "Error while writing:" + e.getMessage());
            return Command.ERROR_SOCKET_ERROR;
        }
    }

    private long closeConnection(String remoteMac)
    {
        Connection conn = m_connections.get(remoteMac);
        if (conn == null)
            return Command.ERROR_CONNECTION_NOT_FOUND;
        try {
            conn.socket.close();
            conn.instream.close();
            conn.outstream.close();
        }
        catch (IOException e) {
            Log.w(TAG, "Error while closing socket: " + e.getMessage());
        }
        m_connections.remove(remoteMac);
        return Command.ERROR_NO_ERROR;
    }

    private long getReadInterval()
    {
        if (m_connections.isEmpty())
            return INITIAL_READ_INTERVAL_MS;
        return INITIAL_READ_INTERVAL_MS / m_connections.size();
    }
}
