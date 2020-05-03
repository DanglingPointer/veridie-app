package com.vasilyev.veridie.fragments;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.vasilyev.veridie.R;
import com.vasilyev.veridie.utils.Fonts;

public class IdleFragment extends Fragment
{
    public interface Callbacks
    {
        void onStartConnectingButtonPressed();
    }

    public static IdleFragment newInstance()
    {
        return new IdleFragment();
    }

    private Callbacks m_callbacks;

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState)
    {
        View root = inflater.inflate(R.layout.fragment_idle, container, false);
        Button btn = root.findViewById(R.id.btn_start_connecting);
        btn.setTypeface(Fonts.forText(getContext()));
        btn.setOnClickListener(v -> {
            if (m_callbacks != null)
                m_callbacks.onStartConnectingButtonPressed();
        });
        TextView welcomeTxt = root.findViewById(R.id.txt_welcome_msg);
        welcomeTxt.setTypeface(Fonts.forText(getContext()));
        return root;
    }

    @Override
    public void onAttach(Context context)
    {
        super.onAttach(context);
        m_callbacks = (Callbacks)context;
    }

    @Override
    public void onDetach()
    {
        m_callbacks = null;
        super.onDetach();
    }
}
