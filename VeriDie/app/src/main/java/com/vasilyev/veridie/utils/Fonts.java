package com.vasilyev.veridie.utils;

import android.content.Context;
import android.graphics.Typeface;

public final class Fonts
{
    public static Typeface forText(Context c)
    {
        return Typeface.createFromAsset(c.getAssets(), "kirsty_rg.ttf");
    }

    public static Typeface forTitle(Context c)
    {
        return Typeface.createFromAsset(c.getAssets(), "RioGrande.ttf");
    }
}
