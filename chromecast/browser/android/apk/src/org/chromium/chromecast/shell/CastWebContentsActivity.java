// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.shell;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.content_public.browser.WebContents;

/**
 * Activity for displaying a WebContents in CastShell.
 * <p>
 * Typically, this class is controlled by CastContentWindowAndroid through
 * CastWebContentsSurfaceHelper. CastContentWindowAndroid which will
 * start a new instance of this activity. If the CastContentWindowAndroid is
 * destroyed, CastWebContentsActivity should finish(). Similarily, if this
 * activity is destroyed, CastContentWindowAndroid should be notified by intent.
 */
public class CastWebContentsActivity extends Activity {
    private static final String TAG = "cr_CastWebActivity";
    private static final boolean DEBUG = true;

    private CastWebContentsSurfaceHelper mSurfaceHelper;

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        if (DEBUG) Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);


        if (!CastBrowserHelper.initializeBrowser(getApplicationContext())) {
            Toast.makeText(this, R.string.browser_process_initialization_failed, Toast.LENGTH_SHORT)
                    .show();
            finish();
        }

        // Set flags to both exit sleep mode when this activity starts and
        // avoid entering sleep mode while playing media. We cannot distinguish
        // between video and audio so this applies to both.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.cast_web_contents_activity);

        mSurfaceHelper = new CastWebContentsSurfaceHelper(this, /* hostActivity */
                (FrameLayout) findViewById(R.id.web_contents_container),
                false /* showInFragment */);

        // Receiver to handle on web content stopped event
        BroadcastReceiver stopEventReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (DEBUG) Log.d(TAG, "Intent action=" + intent.getAction());
                getLocalBroadcastManager().unregisterReceiver(this);
                finish();
            }
        };
        IntentFilter stopReceiverFilter = new IntentFilter();
        stopReceiverFilter.addAction(CastIntents.ACTION_ON_WEB_CONTENT_STOPPED);
        getLocalBroadcastManager().registerReceiver(stopEventReceiver, stopReceiverFilter);

        handleIntent(getIntent());
    }

    private LocalBroadcastManager getLocalBroadcastManager() {
        return LocalBroadcastManager.getInstance(ContextUtils.getApplicationContext());
    }

    protected void handleIntent(Intent intent) {
        final Bundle bundle = intent.getExtras();
        if (bundle == null) {
            Log.i(TAG, "Intent without bundle received!");
            return;
        }
        final String uriString = bundle.getString(CastWebContentsComponent.INTENT_EXTRA_URI);
        if (uriString == null) {
            Log.i(TAG, "Intent without uri received!");
            return;
        }
        final Uri uri = Uri.parse(uriString);

        // Do not load the WebContents if we are simply bringing the same
        // activity to the foreground.
        if (mSurfaceHelper.getInstanceId() != null
                && mSurfaceHelper.getInstanceId().equals(uri.getPath())) {
            Log.i(TAG, "Duplicated intent received!");
            return;
        }

        bundle.setClassLoader(WebContents.class.getClassLoader());
        final WebContents webContents = (WebContents) bundle.getParcelable(
                CastWebContentsComponent.ACTION_EXTRA_WEB_CONTENTS);

        boolean touchInputEnabled =
                bundle.getBoolean(CastWebContentsComponent.ACTION_EXTRA_TOUCH_INPUT_ENABLED, false);
        mSurfaceHelper.onNewWebContents(uri, webContents, touchInputEnabled);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (DEBUG) Log.d(TAG, "onNewIntent");

        // If we're currently finishing this activity, we should start a new activity to
        // display the new app.
        if (isFinishing()) {
            Log.d(TAG, "Activity is finishing, starting new activity.");
            int flags = intent.getFlags();
            flags = flags & ~Intent.FLAG_ACTIVITY_SINGLE_TOP;
            intent.setFlags(flags);
            startActivity(intent);
            return;
        }

        handleIntent(intent);
    }


    @Override
    protected void onStart() {
        if (DEBUG) Log.d(TAG, "onStart");
        super.onStart();
    }

    @Override
    protected void onPause() {
        if (DEBUG) Log.d(TAG, "onPause");
        super.onPause();

        if (mSurfaceHelper != null) {
            mSurfaceHelper.onPause();
        }
    }

    @Override
    protected void onResume() {
        if (DEBUG) Log.d(TAG, "onResume");
        super.onResume();
        if (mSurfaceHelper != null) {
            mSurfaceHelper.onResume();
        }
    }

    @Override
    protected void onStop() {
        if (DEBUG) Log.d(TAG, "onStop");
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        if (mSurfaceHelper != null) {
            mSurfaceHelper.onDestroy();
        }
        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (DEBUG) Log.d(TAG, "onWindowFocusChanged(%b)", hasFocus);
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            // switch to fullscreen (immersive) mode
            getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (DEBUG) Log.d(TAG, "dispatchKeyEvent");
        int keyCode = event.getKeyCode();
        int action = event.getAction();

        // Similar condition for all single-click events.
        if (action == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
            if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE
                    || keyCode == KeyEvent.KEYCODE_MEDIA_STOP
                    || keyCode == KeyEvent.KEYCODE_MEDIA_NEXT
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PREVIOUS) {
                CastWebContentsComponent.onKeyDown(this, mSurfaceHelper.getInstanceId(), keyCode);

                // Stop key should end the entire session.
                if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP) {
                    finish();
                }

                return true;
            }
        }

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            return super.dispatchKeyEvent(event);
        }
        return false;
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent ev) {
        return false;
    }

    @Override
    public boolean dispatchKeyShortcutEvent(KeyEvent event) {
        return false;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (mSurfaceHelper.isTouchInputEnabled()) {
            return super.dispatchTouchEvent(ev);
        } else {
            return false;
        }
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent ev) {
        return false;
    }
}
