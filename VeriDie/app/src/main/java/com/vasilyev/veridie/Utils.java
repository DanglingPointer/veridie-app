package com.vasilyev.veridie;

import android.content.Context;
import android.graphics.Typeface;

public final class Utils
{
    public static Typeface getTextFont(Context c)
    {
        return Typeface.createFromAsset(c.getAssets(), "kirsty_rg.ttf");
    }

    public static Typeface getTitleFont(Context c)
    {
        return Typeface.createFromAsset(c.getAssets(), "RioGrande.ttf");
    }
}
