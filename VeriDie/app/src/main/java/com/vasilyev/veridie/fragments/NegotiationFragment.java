package com.vasilyev.veridie.fragments;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.vasilyev.veridie.R;
import com.vasilyev.veridie.utils.Fonts;

public class NegotiationFragment extends Fragment
{
    public static NegotiationFragment newInstance()
    {
        return new NegotiationFragment();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState)
    {
        View root = inflater.inflate(R.layout.fragment_negotiation, container, false);
        TextView waitMessage = root.findViewById(R.id.txt_wait_message);
        waitMessage.setTypeface(Fonts.forText(getActivity()));
        return root;
    }
}
