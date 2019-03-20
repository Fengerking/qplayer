package com.qiniu.qplayer.mediaEngine;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.CellSignalStrengthCdma;
import android.telephony.CellSignalStrengthGsm;
import android.telephony.CellSignalStrengthLte;
import android.telephony.CellSignalStrengthWcdma;
import android.telephony.TelephonyManager;
import android.util.DisplayMetrics;
import android.util.Pair;
import android.view.Display;
import android.view.WindowManager;

import java.lang.reflect.Method;
import java.util.List;
import java.util.Random;

/**
 * An helper class
 */
public final class BaseUtil {
    /**
     * Like {@link Build.VERSION#SDK_INT}, but in a place where it can be conveniently
     * overridden for local testing.
     */
    public static final int SDK_INT = Build.VERSION.SDK_INT;

    /**
     * Returns a user agent string based on the given application name and the library version.
     *
     * @param context         A valid context of the calling application.
     * @param applicationName String that will be prefix'ed to the generated user agent.
     * @return A user agent string generated using the applicationName and the library version.
     */
    public static String getUserAgent(Context context, String applicationName) {
        String versionName;
        try {
            String packageName = context.getPackageName();
            PackageInfo info = context.getPackageManager().getPackageInfo(packageName, 0);
            versionName = info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            versionName = "?";
        }
        return applicationName + "/" + versionName + " (Linux;Android " + Build.VERSION.RELEASE + ") "; //+ "PLDroidPlayer/" + Config.VERSION;
    }

    /**
     * Gets the resolution,
     *
     * @param ctx the ctx
     * @return a pair to return the width and height
     */
    public static Pair<Integer, Integer> getResolution(Context ctx) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            return getRealResolution(ctx);
        } else {
            return getRealResolutionOnOldDevice(ctx);
        }
    }

    /**
     * Gets resolution on old devices.
     * Tries the reflection to get the real resolution first.
     * Fall back to getDisplayMetrics if the above method failed.
     */
    private static Pair<Integer, Integer> getRealResolutionOnOldDevice(Context ctx) {
        try {
            WindowManager wm = (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
            Display display = wm.getDefaultDisplay();
            Method mGetRawWidth = Display.class.getMethod("getRawWidth");
            Method mGetRawHeight = Display.class.getMethod("getRawHeight");
            Integer realWidth = (Integer) mGetRawWidth.invoke(display);
            Integer realHeight = (Integer) mGetRawHeight.invoke(display);
            return new Pair<>(realWidth, realHeight);
        } catch (Exception e) {
            DisplayMetrics disp = ctx.getResources().getDisplayMetrics();
            return new Pair<>(disp.widthPixels, disp.heightPixels);
        }
    }

    /**
     * Gets real resolution via the new getRealMetrics API.
     */
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    private static Pair<Integer, Integer> getRealResolution(Context ctx) {
        WindowManager wm = (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        DisplayMetrics metrics = new DisplayMetrics();
        display.getRealMetrics(metrics);
        return new Pair<>(metrics.widthPixels, metrics.heightPixels);
    }

    /**
     * Is url local file boolean.
     *
     * @param path the path
     * @return the boolean
     */
    public static boolean isUrlLocalFile(String path) {
        return getPathScheme(path) == null || "file".equals(getPathScheme(path));
    }

    /**
     * Gets path scheme.
     *
     * @param path the path
     * @return the path scheme
     */
    public static String getPathScheme(String path) {
        return Uri.parse(path).getScheme();
    }

    /**
     * Is live streaming boolean.
     *
     * @param url the url
     * @return the boolean
     */
    @Deprecated
    public static boolean isLiveStreaming(String url) {
        if (url.startsWith("rtmp://") || url.endsWith(".m3u8")) {
            return true;
        }
        return false;
    }

    /**
     * Gets display default rotation.
     *
     * @param ctx the ctx
     * @return the display default rotation
     */
    public static int getDisplayDefaultRotation(Context ctx) {
        WindowManager windowManager = (WindowManager) ctx.getSystemService(Context.WINDOW_SERVICE);
        return windowManager.getDefaultDisplay().getRotation();
    }

    /**
     * Is network connected.
     *
     * @param context the context
     * @return the boolean
     */
    public static boolean isNetworkConnected(Context context) {
        if (context == null) {
            return false;
        }
        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
        return networkInfo != null && networkInfo.isAvailable();
    }

    public static String netType(Context appContext) {
        if (appContext == null) {
            return "";
        }
        ConnectivityManager connectivityManager = (ConnectivityManager) appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager == null) {
            return "None";
        }
        try {
            NetworkInfo netInfo = connectivityManager.getActiveNetworkInfo();
            if (netInfo == null || !netInfo.isConnected()) {
                return "None";
            }

            if (netInfo.getType() == ConnectivityManager.TYPE_WIFI) {
                return "WIFI";
            }

            return netInfo.getSubtypeName();
        } catch (Exception e) { // permission denied
            return "Unknown";
        }
    }

    public static boolean getWifiPermission(Context context) {
        boolean ret = false;
        if (context != null) {
            ret = context.checkCallingOrSelfPermission("android.permission.ACCESS_WIFI_STATE")
                    == PackageManager.PERMISSION_GRANTED;
        }
        return ret;
    }

    public static String[] getWifiInfo(Context context) {
        String[] ret = new String[]{"", ""};
        if (getWifiPermission(context)) {
            WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if (wifiManager != null) {
                WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                if (wifiInfo != null) {
                    String ssid = wifiInfo.getSSID();
                    //int level = wifiManager.calculateSignalLevel(wifiInfo.getRssi(), 5);
                    ret = new String[]{ssid, Integer.toString(wifiInfo.getRssi())};
                }
            }
        }
        return ret;
    }

    public static String[] getPhoneInfo(Context context) {
        String[] ret = new String[]{"", ""};
        if (context != null && getPhonePermission(context) && Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN_MR1) {
            TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            if (telephonyManager == null) {
                return ret;
            }
            ret[0] = telephonyManager.getNetworkOperatorName();
            //This will give info of all sims present inside your mobile
            try {
                List<CellInfo> cellInfos = telephonyManager.getAllCellInfo();
                if (cellInfos != null) {
                    for (int i = 0; i < cellInfos.size(); i++) {
                        if (cellInfos.get(i).isRegistered()) {
                            CellInfo cellInfo = cellInfos.get(i);
                            if (cellInfo instanceof CellInfoCdma) {
                                CellInfoCdma cellInfoCdma = (CellInfoCdma) cellInfo;
                                CellSignalStrengthCdma cellSignalStrengthCdma = cellInfoCdma.getCellSignalStrength();
                                ret[1] = String.valueOf(cellSignalStrengthCdma.getLevel());
                            } else if (cellInfo instanceof CellInfoWcdma) {
                                CellInfoWcdma cellInfoWcdma = (CellInfoWcdma) cellInfo;
                                CellSignalStrengthWcdma cellSignalStrengthWcdma = cellInfoWcdma.getCellSignalStrength();
                                ret[1] = String.valueOf(cellSignalStrengthWcdma.getLevel());
                            } else if (cellInfo instanceof CellInfoGsm) {
                                CellInfoGsm cellInfogsm = (CellInfoGsm) cellInfo;
                                CellSignalStrengthGsm cellSignalStrengthGsm = cellInfogsm.getCellSignalStrength();
                                ret[1] = String.valueOf(cellSignalStrengthGsm.getLevel());
                            } else if (cellInfo instanceof CellInfoLte) {
                                CellInfoLte cellInfoLte = (CellInfoLte) cellInfo;
                                CellSignalStrengthLte cellSignalStrengthLte = cellInfoLte.getCellSignalStrength();
                                ret[1] = String.valueOf(cellSignalStrengthLte.getLevel());
                            }
                        }
                    }
                }
            } catch (SecurityException e) {
                return ret;
            }
        }
        return ret;
    }

    public static boolean getPhonePermission(Context context) {
        boolean ret = false;
        if (context != null) {
            ret = context.checkCallingOrSelfPermission("android.permission.READ_PHONE_STATE")
                    == PackageManager.PERMISSION_GRANTED
                    && context.checkCallingOrSelfPermission("android.permission.ACCESS_COARSE_LOCATION")
                    == PackageManager.PERMISSION_GRANTED;
        }
        return ret;
    }

    public static String replaceNull(String str) {
        if (str == null || "".equals(str)) {
            return "-";
        }
        return str;
    }

    public static boolean isNumber(String str) {
        return str != null && str.matches("^(\\-|\\+)?\\d+(\\.\\d+)?$");
    }

    public static String getDeviceId(Context context) {
        SharedPreferences preferences = context.getSharedPreferences("qos", Context.MODE_PRIVATE);
        String deviceId = preferences.getString("deviceId", "");
        if ("".equals(deviceId)) {
            deviceId = deviceId();
            SharedPreferences.Editor editor = preferences.edit();
            editor.putString("deviceId", deviceId);
            editor.commit();
        }
        return deviceId;
    }

    public static String deviceId() {
        Random r = new Random();
        return System.currentTimeMillis() + "" + r.nextInt(999);
    }

    public static String appName(Context context) {
        PackageInfo info;
        try {
            info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return info.packageName;
        } catch (PackageManager.NameNotFoundException e) {
            // e.printStackTrace();
        }
        return "";
    }

    public static String appVersion(Context context) {
        PackageInfo info;
        try {
            info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            // e.printStackTrace();
        }
        return "";
    }
}
