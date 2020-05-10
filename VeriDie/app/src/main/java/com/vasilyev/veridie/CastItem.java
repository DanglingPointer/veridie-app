package com.vasilyev.veridie;

import android.os.Parcel;
import android.os.Parcelable;

import com.vasilyev.veridie.utils.Cast;

public class CastItem implements Parcelable
{
    public static final Parcelable.Creator<CastItem> CREATOR = new Parcelable.Creator<CastItem>()
    {
        @Override
        public CastItem createFromParcel(Parcel source)
        {
            String from = source.readString();
            int what = source.readInt();
            switch (what) {
            case IS_REQUEST_FLAG:
                return new CastItem(new Cast.Request(source), from);
            case IS_RESULT_FLAG:
                return new CastItem(new Cast.Result(source), from);
            default:
                return null;
            }
        }
        @Override
        public CastItem[] newArray(int size)
        {
            return new CastItem[size];
        }
    };

    private static final int IS_REQUEST_FLAG = 0;
    private static final int IS_RESULT_FLAG = 1;

    private final Object m_cast;
    private final String m_from;

    public CastItem(Cast.Request request, String author)
    {
        m_cast = request;
        m_from = author;
    }
    public CastItem(Cast.Result response, String generator)
    {
        m_cast = response;
        m_from = generator;
    }
    public Object getCast()
    {
        return m_cast;
    }
    public String getFrom()
    {
        return m_from;
    }
    @Override
    public int describeContents()
    {
        return 0;
    }
    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
        dest.writeString(m_from);
        if (m_cast instanceof Cast.Request) {
            dest.writeInt(IS_REQUEST_FLAG);
            Cast.Request request = (Cast.Request)m_cast;
            request.toParcel(dest);
        } else if (m_cast instanceof Cast.Result) {
            dest.writeInt(IS_RESULT_FLAG);
            Cast.Result result = (Cast.Result)m_cast;
            result.toParcel(dest);
        } else {
            throw new IllegalStateException("No cast in CastItem");
        }
    }
}
