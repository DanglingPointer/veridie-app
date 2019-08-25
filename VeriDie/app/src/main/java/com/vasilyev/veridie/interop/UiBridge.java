package com.vasilyev.veridie.interop;

import com.vasilyev.veridie.CastRequest;

public class UiBridge
{
    public static final int NO_ERROR = 0;
    public static final int ERROR_NO_LISTENER = 1;
    public static final int ERROR_UNHANDLED_EXCEPTION = 2;


    private final static String TAG = BluetoothBridge.class.getName();

    public interface Listener
    {
        // TODO
    }
    private static UiBridge s_instance = null;
    public static UiBridge getInstance() {
        if (s_instance == null)
            s_instance = new UiBridge();
        return s_instance;
    }

    private static Listener m_listener;
    private UiBridge() {}
    public void setListener(Listener l) {
        bridgeCreated();
        m_listener = l;
    }

    /** C++ --> JAVA */

    // TODO

    /** Java --> C++*/

    public void queryConnectedDevices() {
        queryDevices(true, false);
    }
    public void queryDiscoveredDevices() {
        queryDevices(false, true);
    }
    public void setPlayerName(String name) {
        setName(name);
    }
    public void queryPlayerName() {
        queryLocalName();
    }
    public void requestCast(CastRequest request) {
        castRequest(request.getD(), request.getCount(),
                request.getThreshold() == null ? -1 : request.getThreshold());
    }
    public void setApprovedCandidate(String approvedMacAddrs) {
        candidateApproved(approvedMacAddrs);
    }
    public void startNewGame() {
        newGame();
    }
    public void restoreState() {
        restoringState();
    }
    public void saveState() {
        savingState();
    }

    private static native void bridgeCreated();
    private static native void queryDevices(boolean connected, boolean discovered);
    private static native void setName(String name);
    private static native void queryLocalName();
    private static native void castRequest(int d, int count, int threshold);
    private static native void candidateApproved(String approvedMac);
    private static native void newGame();
    private static native void restoringState();
    private static native void savingState();
}
