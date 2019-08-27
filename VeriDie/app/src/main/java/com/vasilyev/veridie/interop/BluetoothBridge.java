package com.vasilyev.veridie.interop;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.util.Log;

import java.util.Collection;
import java.util.UUID;

import static android.bluetooth.BluetoothAdapter.SCAN_MODE_CONNECTABLE;
import static android.bluetooth.BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE;
import static android.bluetooth.BluetoothAdapter.SCAN_MODE_NONE;

public class BluetoothBridge
{
    public static final int NO_ERROR = 0;
    public static final int ERROR_NO_LISTENER = 1;
    public static final int ERROR_UNHANDLED_EXCEPTION = 2;
    public static final int ERROR_WRONG_STATE = 3;

    private static final String TAG = BluetoothBridge.class.getName();

    public interface Listener
    {
        boolean isBluetoothEnabled();
        Collection<BluetoothDevice> getPairedDevices();
        int startDiscovery();
        int cancelDiscovery();
        int requestDiscoverability(int durationSec);
        int startListening(String name, UUID id);
        int stopListening();
        int connect(String serverMac, UUID conn);
        int closeConnection(String remoteMac);
        int sendMessage(String remoteMac, byte[] msg);
    }

    private static BluetoothBridge s_instance = null;
    public static BluetoothBridge getInstance() {
        if (s_instance == null)
            s_instance = new BluetoothBridge();
        return s_instance;
    }

    private Listener m_listener;
    private BluetoothBridge() {}
    public void setListener(Listener l) {
        bridgeCreated();
        m_listener = l;
    }

    /** C++ --> JAVA */

    private static boolean isBluetoothEnabled() {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return false;
            return l.isBluetoothEnabled();
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return false;
        }
    }
    private static int requestPairedDevices() {
        try {
            Listener l = s_instance.m_listener;
            if (l == null) {
                return ERROR_NO_LISTENER;
            }
            Collection<BluetoothDevice> paired = l.getPairedDevices();
            if (paired == null) {
                return ERROR_WRONG_STATE;
            }
            for (BluetoothDevice d : paired) {
                deviceFound(d.getName(), d.getAddress(), true);
            }
            return NO_ERROR;
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int startDiscovery() {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.startDiscovery();
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int cancelDiscovery() {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.cancelDiscovery();
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int requestDiscoverability(int sec) {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.requestDiscoverability(sec);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int startListening(String name, long uuidLsl, long uuidMsl) {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.startListening(name, new UUID(uuidMsl, uuidLsl));
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int stopListening() {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.stopListening();
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int connect(String serverMac, long uuidLsl, long uuidMsl) {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.connect(serverMac, new UUID(uuidMsl, uuidLsl));
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int closeConnection(String remoteMac) {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.closeConnection(remoteMac);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }
    private static int sendMessage(String remoteMac, byte[] msg) {
        try {
            Listener l = s_instance.m_listener;
            if (l == null)
                return ERROR_NO_LISTENER;
            return l.sendMessage(remoteMac, msg);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }

    /** JAVA --> C++ */

    public void onOffState(boolean on) {
        if (on)
            bluetoothOn();
        else
            bluetoothOff();
    }
    public void deviceFound(BluetoothDevice device) {
        deviceFound(device.getName(), device.getAddress(), false);
    }
    public void discoverabilityResponse(boolean confirmed) {
        if (confirmed)
            discoverabilityConfirmed();
        else
            discoverabilityRejected();
    }
    public void discoverabilityChanged(int mode) {
        switch (mode) {
            case SCAN_MODE_CONNECTABLE_DISCOVERABLE:
                scanModeChanged(true, true);
                break;
            case SCAN_MODE_CONNECTABLE:
                scanModeChanged(false, true);
                break;
            case SCAN_MODE_NONE:
                scanModeChanged(false, false);
                break;
        }
    }
    public void deviceConnected(BluetoothDevice device) {
        deviceConnected(device.getName(), device.getAddress());
    }
    public void deviceDisonnected(BluetoothDevice device) {
        deviceDisonnected(device.getAddress());
    }

    public void messageReceived(BluetoothSocket socket, byte[] msg, int length) {
        messageReceived(socket.getRemoteDevice().getAddress(), msg, length);
    }

    private static native void bridgeCreated();
    private static native void bluetoothOn();
    private static native void bluetoothOff();
    private static native void deviceFound(String name, String mac, boolean paired);
    private static native void discoverabilityConfirmed();
    private static native void discoverabilityRejected();
    private static native void scanModeChanged(boolean discoverable, boolean connectable);
    private static native void deviceConnected(String name, String mac);
    private static native void deviceDisonnected(String mac);
    private static native void messageReceived(String remoteMac, byte[] msg, int length);
}
