package com.chess_ar.handtracking.archess;

public class JNIInterface {
    public native static long createContext();
    public native static long deleteContext(long id);
}
