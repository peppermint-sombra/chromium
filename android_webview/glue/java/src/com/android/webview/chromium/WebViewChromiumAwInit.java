// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.Manifest;
import android.app.ActivityManager;
import android.content.ComponentCallbacks2;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Looper;
import android.os.Process;
import android.util.Log;
import android.webkit.CookieManager;
import android.webkit.GeolocationPermissions;
import android.webkit.ServiceWorkerController;
import android.webkit.TokenBindingService;
import android.webkit.ValueCallback;
import android.webkit.WebStorage;
import android.webkit.WebViewDatabase;
import android.webkit.WebViewFactory;
import android.webkit.WebViewFactoryProvider;
import android.webkit.WebViewFactoryProvider.Statics;

import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwBrowserProcess;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwContentsClient;
import org.chromium.android_webview.AwContentsStatics;
import org.chromium.android_webview.AwCookieManager;
import org.chromium.android_webview.AwDevToolsServer;
import org.chromium.android_webview.AwNetworkChangeNotifierRegistrationPolicy;
import org.chromium.android_webview.AwQuotaManagerBridge;
import org.chromium.android_webview.AwResource;
import org.chromium.android_webview.AwSettings;
import org.chromium.android_webview.AwSwitches;
import org.chromium.android_webview.AwTracingController;
import org.chromium.android_webview.HttpAuthDatabase;
import org.chromium.android_webview.command_line.CommandLineUtil;
import org.chromium.android_webview.variations.AwVariationsSeedHandler;
import org.chromium.base.BuildConfig;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.PathService;
import org.chromium.base.ThreadUtils;
import org.chromium.base.TraceEvent;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.net.NetworkChangeNotifier;

import java.util.List;

class WebViewChromiumAwInit {

    private static final String TAG = "WebViewChromiumAwInit";

    private static final String HTTP_AUTH_DATABASE_FILE = "http_auth.db";

    // TODO(gsennton): store aw-objects instead of adapters here
    // Initialization guarded by mLock.
    private AwBrowserContext mBrowserContext;
    private Statics mStaticMethods;
    private GeolocationPermissionsAdapter mGeolocationPermissions;
    private CookieManagerAdapter mCookieManager;
    private Object mTokenBindingManager;
    private WebIconDatabaseAdapter mWebIconDatabase;
    private WebStorageAdapter mWebStorage;
    private WebViewDatabaseAdapter mWebViewDatabase;
    private AwDevToolsServer mDevToolsServer;
    private Object mServiceWorkerController;

    // Guards accees to the other members, and is notifyAll() signalled on the UI thread
    // when the chromium process has been started.
    // This member is not private only because the downstream subclass needs to access it,
    // it shouldn't be accessed from anywhere else.
    /* package */ final Object mLock = new Object();

    // Read/write protected by mLock.
    private boolean mStarted;

    private final WebViewChromiumFactoryProvider mFactory;

    WebViewChromiumAwInit(WebViewChromiumFactoryProvider factory) {
        mFactory = factory;
        // Do not make calls into 'factory' in this ctor - this ctor is called from the
        // WebViewChromiumFactoryProvider ctor, so 'factory' is not properly initialized yet.
    }

    AwTracingController getTracingControllerOnUiThread() {
        synchronized (mLock) {
            ensureChromiumStartedLocked(true);
            return getBrowserContextOnUiThread().getTracingController();
        }
    }

    // TODO: DIR_RESOURCE_PAKS_ANDROID needs to live somewhere sensible,
    // inlined here for simplicity setting up the HTMLViewer demo. Unfortunately
    // it can't go into base.PathService, as the native constant it refers to
    // lives in the ui/ layer. See ui/base/ui_base_paths.h
    private static final int DIR_RESOURCE_PAKS_ANDROID = 3003;

    protected void startChromiumLocked() {
        assert Thread.holdsLock(mLock) && ThreadUtils.runningOnUiThread();

        // The post-condition of this method is everything is ready, so notify now to cover all
        // return paths. (Other threads will not wake-up until we release |mLock|, whatever).
        mLock.notifyAll();

        if (mStarted) {
            return;
        }

        try {
            LibraryLoader.get(LibraryProcessType.PROCESS_WEBVIEW).ensureInitialized();
        } catch (ProcessInitException e) {
            throw new RuntimeException("Error initializing WebView library", e);
        }

        PathService.override(PathService.DIR_MODULE, "/system/lib/");
        PathService.override(DIR_RESOURCE_PAKS_ANDROID, "/system/framework/webview/paks");

        // Make sure that ResourceProvider is initialized before starting the browser process.
        final PackageInfo webViewPackageInfo = WebViewFactory.getLoadedPackageInfo();
        final String webViewPackageName = webViewPackageInfo.packageName;
        final Context context = ContextUtils.getApplicationContext();
        setUpResources(webViewPackageInfo, context);
        initPlatSupportLibrary();
        doNetworkInitializations(context);
        final boolean isExternalService = true;
        // The WebView package name is used to locate the separate Service to which we copy crash
        // minidumps. This package name must be set before a render process has a chance to crash -
        // otherwise we might try to copy a minidump without knowing what process to copy it to.
        // It's also used to determine channel for UMA, so it must be set before initializing UMA.
        AwBrowserProcess.setWebViewPackageName(webViewPackageName);
        AwBrowserProcess.configureChildProcessLauncher(webViewPackageName, isExternalService);
        AwBrowserProcess.start();
        AwBrowserProcess.handleMinidumpsAndSetMetricsConsent(
                webViewPackageName, true /* updateMetricsConsent */);

        if (CommandLineUtil.isBuildDebuggable()) {
            setWebContentsDebuggingEnabled(true);
        }

        TraceEvent.setATraceEnabled(mFactory.getWebViewDelegate().isTraceTagEnabled());
        mFactory.getWebViewDelegate().setOnTraceEnabledChangeListener(
                new WebViewDelegate.OnTraceEnabledChangeListener() {
                    @Override
                    public void onTraceEnabledChange(boolean enabled) {
                        TraceEvent.setATraceEnabled(enabled);
                    }
                });

        mStarted = true;

        // Initialize thread-unsafe singletons.
        AwBrowserContext awBrowserContext = getBrowserContextOnUiThread();
        mGeolocationPermissions = new GeolocationPermissionsAdapter(
                mFactory, awBrowserContext.getGeolocationPermissions());
        mWebStorage = new WebStorageAdapter(mFactory, AwQuotaManagerBridge.getInstance());
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
            mServiceWorkerController = new ServiceWorkerControllerAdapter(
                    awBrowserContext.getServiceWorkerController());
        }

        mFactory.getRunQueue().drainQueue();

        boolean enableVariations =
                CommandLine.getInstance().hasSwitch(AwSwitches.ENABLE_WEBVIEW_VARIATIONS);
        if (enableVariations) {
            AwVariationsSeedHandler.bindToVariationsService();
        }
    }

    private void setUpResources(PackageInfo webViewPackageInfo, Context context) {
        String packageName = webViewPackageInfo.packageName;
        if (webViewPackageInfo.applicationInfo.metaData != null) {
            packageName = webViewPackageInfo.applicationInfo.metaData.getString(
                    "com.android.webview.WebViewDonorPackage", packageName);
        }
        ResourceRewriter.rewriteRValues(
                mFactory.getWebViewDelegate().getPackageId(context.getResources(), packageName));

        AwResource.setResources(context.getResources());
        AwResource.setConfigKeySystemUuidMapping(android.R.array.config_keySystemUuidMapping);
    }

    boolean hasStarted() {
        return mStarted;
    }

    void startYourEngines(boolean onMainThread) {
        synchronized (mLock) {
            ensureChromiumStartedLocked(onMainThread);
        }
    }

    // This method is not private only because the downstream subclass needs to access it,
    // it shouldn't be accessed from anywhere else.
    /* package */ void ensureChromiumStartedLocked(boolean onMainThread) {
        assert Thread.holdsLock(mLock);

        if (mStarted) { // Early-out for the common case.
            return;
        }

        Looper looper = !onMainThread ? Looper.myLooper() : Looper.getMainLooper();
        Log.v(TAG, "Binding Chromium to "
                        + (Looper.getMainLooper().equals(looper) ? "main" : "background")
                        + " looper " + looper);
        ThreadUtils.setUiThread(looper);

        if (ThreadUtils.runningOnUiThread()) {
            startChromiumLocked();
            return;
        }

        // We must post to the UI thread to cover the case that the user has invoked Chromium
        // startup by using the (thread-safe) CookieManager rather than creating a WebView.
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                synchronized (mLock) {
                    startChromiumLocked();
                }
            }
        });
        while (!mStarted) {
            try {
                // Important: wait() releases |mLock| the UI thread can take it :-)
                mLock.wait();
            } catch (InterruptedException e) {
                // Keep trying... eventually the UI thread will process the task we sent it.
            }
        }
    }

    private void initPlatSupportLibrary() {
        DrawGLFunctor.setChromiumAwDrawGLFunction(AwContents.getAwDrawGLFunction());
        AwContents.setAwDrawSWFunctionTable(GraphicsUtils.getDrawSWFunctionTable());
        AwContents.setAwDrawGLFunctionTable(GraphicsUtils.getDrawGLFunctionTable());
    }

    private void doNetworkInitializations(Context applicationContext) {
        if (applicationContext.checkPermission(Manifest.permission.ACCESS_NETWORK_STATE,
                Process.myPid(), Process.myUid()) == PackageManager.PERMISSION_GRANTED) {
            NetworkChangeNotifier.init();
            NetworkChangeNotifier.setAutoDetectConnectivityState(
                    new AwNetworkChangeNotifierRegistrationPolicy());
        }

        AwContentsStatics.setCheckClearTextPermitted(
                applicationContext.getApplicationInfo().targetSdkVersion >= Build.VERSION_CODES.O);
    }

    // Only on UI thread.
    AwBrowserContext getBrowserContextOnUiThread() {
        assert mStarted;

        if (BuildConfig.DCHECK_IS_ON && !ThreadUtils.runningOnUiThread()) {
            throw new RuntimeException(
                    "getBrowserContextOnUiThread called on " + Thread.currentThread());
        }

        if (mBrowserContext == null) {
            mBrowserContext = new AwBrowserContext(
                mFactory.getWebViewPrefs(), ContextUtils.getApplicationContext());
        }
        return mBrowserContext;
    }

    // TODO(gsennton) create an AwStatics class to return from this class
    public Statics getStatics() {
        synchronized (mLock) {
            if (mStaticMethods == null) {
                // TODO: Optimization potential: most these methods only need the native library
                // loaded and initialized, not the entire browser process started.
                // See also http://b/7009882
                ensureChromiumStartedLocked(true);
                mStaticMethods = new WebViewFactoryProvider.Statics() {
                    @Override
                    public String findAddress(String addr) {
                        return AwContentsStatics.findAddress(addr);
                    }

                    @Override
                    public String getDefaultUserAgent(Context context) {
                        return AwSettings.getDefaultUserAgent();
                    }

                    @Override
                    public void setWebContentsDebuggingEnabled(boolean enable) {
                        // Web Contents debugging is always enabled on debug builds.
                        if (!CommandLineUtil.isBuildDebuggable()) {
                            WebViewChromiumAwInit.this.setWebContentsDebuggingEnabled(
                                    enable);
                        }
                    }

                    @Override
                    public void clearClientCertPreferences(Runnable onCleared) {
                        // clang-format off
                        ThreadUtils.runOnUiThread(() ->
                                AwContentsStatics.clearClientCertPreferences(onCleared));
                        // clang-format on
                    }

                    @Override
                    public void freeMemoryForTests() {
                        if (ActivityManager.isRunningInTestHarness()) {
                            MemoryPressureListener.maybeNotifyMemoryPresure(
                                    ComponentCallbacks2.TRIM_MEMORY_COMPLETE);
                        }
                    }

                    @Override
                    public void enableSlowWholeDocumentDraw() {
                        WebViewChromium.enableSlowWholeDocumentDraw();
                    }

                    @Override
                    public Uri[] parseFileChooserResult(int resultCode, Intent intent) {
                        return AwContentsClient.parseFileChooserResult(resultCode, intent);
                    }

                    /**
                     * Starts Safe Browsing initialization. This should only be called once.
                     * @param context is the application context the WebView will be used in.
                     * @param callback will be called with the value true if initialization is
                     * successful. The callback will be run on the UI thread.
                     */
                    @Override
                    public void initSafeBrowsing(Context context, ValueCallback<Boolean> callback) {
                        // clang-format off
                        ThreadUtils.runOnUiThread(() -> AwContentsStatics.initSafeBrowsing(context,
                                    CallbackConverter.fromValueCallback(callback)));
                        // clang-format on
                    }

                    @Override
                    public void setSafeBrowsingWhitelist(
                            List<String> urls, ValueCallback<Boolean> callback) {
                        // clang-format off
                        ThreadUtils.runOnUiThread(() -> AwContentsStatics.setSafeBrowsingWhitelist(
                                urls, CallbackConverter.fromValueCallback(callback)));
                        // clang-format on
                    }

                    /**
                     * Returns a URL pointing to the privacy policy for Safe Browsing reporting.
                     *
                     * @return the url pointing to a privacy policy document which can be displayed
                     * to users.
                     */
                    @Override
                    public Uri getSafeBrowsingPrivacyPolicyUrl() {
                        return AwContentsStatics.getSafeBrowsingPrivacyPolicyUrl();
                    }
                };
            }
        }
        return mStaticMethods;
    }

    private void setWebContentsDebuggingEnabled(boolean enable) {
        if (Looper.myLooper() != ThreadUtils.getUiThreadLooper()) {
            throw new RuntimeException(
                    "Toggling of Web Contents Debugging must be done on the UI thread");
        }
        if (mDevToolsServer == null) {
            if (!enable) return;
            mDevToolsServer = new AwDevToolsServer();
        }
        mDevToolsServer.setRemoteDebuggingEnabled(enable);
    }

    public GeolocationPermissions getGeolocationPermissions() {
        synchronized (mLock) {
            if (mGeolocationPermissions == null) {
                ensureChromiumStartedLocked(true);
            }
        }
        return mGeolocationPermissions;
    }

    public CookieManager getCookieManager() {
        synchronized (mLock) {
            if (mCookieManager == null) {
                mCookieManager = new CookieManagerAdapter(new AwCookieManager());
            }
        }
        return mCookieManager;
    }

    public ServiceWorkerController getServiceWorkerController() {
        synchronized (mLock) {
            if (mServiceWorkerController == null) {
                ensureChromiumStartedLocked(true);
            }
        }
        return (ServiceWorkerController) mServiceWorkerController;
    }

    public TokenBindingService getTokenBindingService() {
        synchronized (mLock) {
            if (mTokenBindingManager == null) {
                mTokenBindingManager = new TokenBindingManagerAdapter(mFactory);
            }
        }
        return (TokenBindingService) mTokenBindingManager;
    }

    public android.webkit.WebIconDatabase getWebIconDatabase() {
        synchronized (mLock) {
            if (mWebIconDatabase == null) {
                ensureChromiumStartedLocked(true);
                mWebIconDatabase = new WebIconDatabaseAdapter();
            }
        }
        return mWebIconDatabase;
    }

    public WebStorage getWebStorage() {
        synchronized (mLock) {
            if (mWebStorage == null) {
                ensureChromiumStartedLocked(true);
            }
        }
        return mWebStorage;
    }

    public WebViewDatabase getWebViewDatabase(final Context context) {
        synchronized (mLock) {
            if (mWebViewDatabase == null) {
                ensureChromiumStartedLocked(true);
                mWebViewDatabase = new WebViewDatabaseAdapter(
                        mFactory, HttpAuthDatabase.newInstance(context, HTTP_AUTH_DATABASE_FILE));
            }
        }
        return mWebViewDatabase;
    }
}
