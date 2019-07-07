package com.gslam.svar;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Created by zhaoyong on 7/6/19.
 */

public class Svar {
    private long native_ptr;

    static Svar undefined;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("gslamSvar");
    }

    public static native Svar instance();

    public Svar(){
        native_ptr=Undefined();
    }

    public Svar(boolean value){
        native_ptr=create(value);
    }

    public Svar(int value){
        native_ptr=create(value);
    }

    public Svar(double value){
        native_ptr=create(value);
    }

    public Svar(String value){
        native_ptr=create(value);
    }

    public Svar(Object[] value)
    {
        if(value==null)
            native_ptr=createList(0,null);
        else {
            native_ptr = createList(value.length, value);
        }
    }

    public Svar(Map map){
        native_ptr=createMap(map);
    }

    public Svar(Object object){
        if(object==null)
            native_ptr=Null();
        else if(object instanceof Integer){
            native_ptr=create(((Integer) object).intValue());
        }
        else if(object instanceof Double){
            native_ptr=create(((Double) object).doubleValue());
        }
        else if(object instanceof String){
            native_ptr=create(object.toString());
        }
        else if(object instanceof List){
            List value=(List)object;
            if(value==null)
                native_ptr=createList(0,null);
            else
                native_ptr=createList(value.size(),value.toArray());
        }
        else if(object instanceof Map){
            native_ptr=createMap((Map)object);
        }
        else if(object instanceof SvarFunction){
            native_ptr=createFunc((SvarFunction)object);
        }
        else {
            native_ptr=createObject(object);
        }
    }

    public native Svar get(int i);

    public native void add(Svar obj);

    public Svar get(String id){return getMap(id);}
    public native Svar getMap(String id);

    public native void set(String id,Svar obj);

    public native void clear();
    public native void erase(String id);

    public native long createList(int length,Object[] list);

    public  long createMap(Map map)
    {
        return 0;
    }

    public  long createObject(Object value)
    {
        return 0;
    }

    public long createFunc(SvarFunction value){
        return 0;
    }

    public Svar call(Object... args){
        List list=new ArrayList<Svar>();
        for(Object obj:args){
            list.add(new Svar(obj));
        }
        return callNative(new Svar(list.toArray()));
    }

    public Svar call(String name,Object... args){
        List list=new ArrayList<Svar>();
        for(Object obj:args){
            list.add(new Svar(obj));
        }
        return callNative(name,new Svar(list.toArray()));
    }

    public native String  type();
    public native String  toString();
    public native int     length();

    public native boolean isClass();
    public native boolean isFunction();
    public native boolean isList();
    public native boolean isMap();
    public native boolean isUndefined();
    public native boolean isNull();
    public native boolean isBoolean();
    public native boolean isChar();
    public native boolean isByte();
    public native boolean isShort();
    public native boolean isInt();
    public native boolean isDouble();
    public native boolean isJavaObject();

    private native long  Undefined();
    private native long  Null();
    private native long  create(boolean value);
    private native long  create(int value);
    private native long  create(double value);
    private native long  create(String value);

    private native Svar callNative(Svar args);
    private native Svar callNative(String name,Svar args);

    private native static void  release(long ptr);

    public static Svar createFromNative(long ptr){
        Svar var=new Svar();
        release(var.native_ptr);
        var.native_ptr=ptr;
        return var;
    }

    protected void finalize(){
        release(native_ptr);
    }

    public interface SvarFunction{
        public Svar call(Svar args);
    };
}
