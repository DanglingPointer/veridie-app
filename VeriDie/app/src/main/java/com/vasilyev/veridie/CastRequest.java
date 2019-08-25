package com.vasilyev.veridie;

public class CastRequest
{
    public static CastRequest newD4CastRequest(int count, Integer threshold) {
        return new CastRequest(4, count, threshold);
    }
    public static CastRequest newD6CastRequest(int count, Integer threshold) {
        return new CastRequest(6, count, threshold);
    }
    public static CastRequest newD8CastRequest(int count, Integer threshold) {
        return new CastRequest(8, count, threshold);
    }
    public static CastRequest newD10CastRequest(int count, Integer threshold) {
        return new CastRequest(10, count, threshold);
    }
    public static CastRequest newD12CastRequest(int count, Integer threshold) {
        return new CastRequest(12, count, threshold);
    }
    public static CastRequest newD16CastRequest(int count, Integer threshold) {
        return new CastRequest(16, count, threshold);
    }
    public static CastRequest newD20CastRequest(int count, Integer threshold) {
        return new CastRequest(20, count, threshold);
    }
    public static CastRequest newD100CastRequest(int count, Integer threshold) {
        return new CastRequest(100, count, threshold);
    }

    private int m_d;
    private int m_count;
    private Integer m_threshold;

    public CastRequest(int d, int count, Integer threshold) {
        m_d = d;
        if (count < 1)
            throw new RuntimeException("Invalid dice count");
        m_count = count;
        if (threshold != null && threshold < d)
            throw new RuntimeException("Invalid thresholde");
        m_threshold = threshold;
    }
    public int getD() {
        return m_d;
    }
    public int getCount() {
        return m_count;
    }
    public Integer getThreshold() {
        return m_threshold;
    }
}
