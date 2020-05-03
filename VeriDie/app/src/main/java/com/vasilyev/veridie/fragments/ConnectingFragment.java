package com.vasilyev.veridie.fragments;

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import com.vasilyev.veridie.R;
import com.vasilyev.veridie.utils.Fonts;

import java.util.ArrayList;

public class ConnectingFragment extends Fragment
{
    public interface Callbacks
    {
        void onConnectDevicePressed(BluetoothDevice device);
        void onDisconnectDevicePressed(BluetoothDevice device);
        void onFinishedConnecting();
    }

    public static ConnectingFragment newInstance()
    {
        return new ConnectingFragment();
    }

    private static final String TAG = "ConnectingFragment";
    private static final String ARG_DISCOVERED_DEVICES = "ARG_DISCOVERED_ARRAY";
    private static final String ARG_CONNECTED_DEVICES = "ARG_CONNECTED_ARRAY";

    private DevicesListAdapter m_discoveredAdapter;
    private DevicesListAdapter m_connectedAdapter;
    private Callbacks m_callbacks;

    public ConnectingFragment()
    {
    }

    public void addDiscoveredDevice(BluetoothDevice device)
    {
        if (!m_discoveredAdapter.getDevices().contains(device))
            m_discoveredAdapter.add(device);
    }

    public void removeDiscoveredDevice(BluetoothDevice device)
    {
        m_discoveredAdapter.remove(device);
    }

    public void addConnectedDevice(BluetoothDevice device)
    {
        m_connectedAdapter.add(device);
    }

    public void removeConnectedDevice(BluetoothDevice device)
    {
        m_connectedAdapter.remove(device);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState)
    {
        View root = inflater.inflate(R.layout.fragment_connecting, container, false);
        ArrayList<BluetoothDevice> discoveredDevices = null;
        ArrayList<BluetoothDevice> connectedDevices = null;
        if (savedInstanceState != null) {
            discoveredDevices = savedInstanceState.getParcelableArrayList(ARG_DISCOVERED_DEVICES);
            connectedDevices = savedInstanceState.getParcelableArrayList(ARG_CONNECTED_DEVICES);
        }
        if (discoveredDevices == null)
            discoveredDevices = new ArrayList<>();
        if (connectedDevices == null)
            connectedDevices = new ArrayList<>();

        m_discoveredAdapter = new DevicesListAdapter(getActivity(), discoveredDevices, false);
        m_connectedAdapter = new DevicesListAdapter(getActivity(), connectedDevices, true);

        ListView discoveredList = root.findViewById(R.id.list_discovered);
        discoveredList.setAdapter(m_discoveredAdapter);
        ListView connectedList = root.findViewById(R.id.list_connected);
        connectedList.setAdapter(m_connectedAdapter);

        TextView title = root.findViewById(R.id.txt_connecting_title);
        title.setTypeface(Fonts.forText(getActivity()));
        TextView discoveredTitle = root.findViewById(R.id.txt_discovered_title);
        discoveredTitle.setTypeface(Fonts.forText(getActivity()));
        TextView connectedTitle = root.findViewById(R.id.txt_connected_title);
        connectedTitle.setTypeface(Fonts.forText(getActivity()));

        Button finishedConnecting = root.findViewById(R.id.btn_finished_connecting);
        finishedConnecting.setTypeface(Fonts.forText(getActivity()));
        finishedConnecting.setOnClickListener((v) -> {
            m_callbacks.onFinishedConnecting();
            finishedConnecting.setEnabled(false);
        });

        return root;
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putParcelableArrayList(ARG_DISCOVERED_DEVICES, m_discoveredAdapter.getDevices());
        outState.putParcelableArrayList(ARG_CONNECTED_DEVICES, m_connectedAdapter.getDevices());
    }

    @Override
    public void onAttach(Context context)
    {
        m_callbacks = (Callbacks)context;
        super.onAttach(context);
    }

    @Override
    public void onDetach()
    {
        super.onDetach();
        m_callbacks = null;
    }

    private static class DevicesListAdapter extends ArrayAdapter<BluetoothDevice>
    {
        private class ViewHolder
        {
            private final TextView m_txtDeviceName;
            private final Button m_btnConnectDisconnect;
            private BluetoothDevice m_lastDevice;

            ViewHolder(View root)
            {
                m_txtDeviceName = root.findViewById(R.id.txt_device_name);
                m_txtDeviceName.setTypeface(Fonts.forText(getContext()));
                m_btnConnectDisconnect = root.findViewById(R.id.btn_connect_disconnect);
                m_btnConnectDisconnect.setTypeface(Fonts.forText(getContext()));
            }

            void bind(BluetoothDevice device)
            {
                if (device.equals(m_lastDevice))
                    return;

                m_btnConnectDisconnect.setEnabled(true);
                if (device.getBondState() == BluetoothDevice.BOND_BONDING)
                    m_btnConnectDisconnect.setText("...");
                else
                    m_btnConnectDisconnect.setText(m_connected
                            ? getContext().getString(R.string.conn_btn_disconnect)
                            : getContext().getString(R.string.conn_btn_connect));

                m_txtDeviceName.setText(device.getName());
                if (m_connected)
                    m_btnConnectDisconnect.setOnClickListener((view) -> {
                        m_callbacks.onDisconnectDevicePressed(device);
                        m_btnConnectDisconnect.setEnabled(false);
                    });
                else
                    m_btnConnectDisconnect.setOnClickListener((view) -> {
                        m_callbacks.onConnectDevicePressed(device);
                        m_btnConnectDisconnect.setEnabled(false);
                        m_btnConnectDisconnect.setText("...");
                    });
                m_lastDevice = device;
            }
        }


        private final ConnectingFragment.Callbacks m_callbacks;
        private final ArrayList<BluetoothDevice> m_items;
        private final boolean m_connected;

        DevicesListAdapter(@NonNull Context context, @NonNull ArrayList<BluetoothDevice> objects, boolean connected)
        {
            super(context, R.layout.list_item_connecting, objects);
            m_callbacks = (ConnectingFragment.Callbacks)context;
            m_items = objects;
            m_connected = connected;
        }

        ArrayList<BluetoothDevice> getDevices()
        {
            return m_items;
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent)
        {
            ViewHolder holder;
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(R.layout.list_item_connecting, parent, false);
                holder = new ViewHolder(convertView);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder)convertView.getTag();
            }
            holder.bind(getItem(position));
            return convertView;
        }
    }
}
