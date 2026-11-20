package io.github.rask_project.metronom;

import android.Manifest;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;
import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;
import androidx.core.content.ContextCompat;
import org.qtproject.qt.android.bindings.QtActivity;

// Keeps the process, and so the audio, alive while the metronome plays in the background. The notification stays
// while paused, with a button to resume, until it is dismissed or the app is closed.
public class MetronomeService extends Service {

    private static final String CHANNEL_ID = "playback";
    private static final int NOTIFICATION_ID = 1;
    private static final String ACTION_SHOW = "io.github.rask_project.metronom.SHOW";
    private static final String ACTION_PLAY = "io.github.rask_project.metronom.PLAY";
    private static final String ACTION_STOP = "io.github.rask_project.metronom.STOP";
    private static final String ACTION_DISMISS = "io.github.rask_project.metronom.DISMISS";
    private static final String EXTRA_PLAYING = "playing";
    private static final String EXTRA_TEXT = "text";
    private static final String EXTRA_NOTE_VALUE = "noteValue";
    private static final String EXTRA_PLAY_TEXT = "playText";
    private static final String EXTRA_STOP_TEXT = "stopText";

    // Indexed by Metronome::NoteValue.
    private static final int[] NOTE_ICONS = {
        R.drawable.ic_note_quarter, R.drawable.ic_note_eighth, R.drawable.ic_note_sixteenth,
        R.drawable.ic_note_thirty_second, R.drawable.ic_note_half, R.drawable.ic_note_dotted_quarter,
    };

    // Set once the app is closed from the recents, so a late update does not bring the notification back.
    private static boolean sClosed = false;

    private boolean mPlaying = false;
    private String mText;
    private int mNoteValue = 0;
    private String mPlayText;
    private String mStopText;

    private static native void nativePlay();
    private static native void nativeStop();

    public static void requestNotificationPermission(Activity activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
                && activity.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)
                        != PackageManager.PERMISSION_GRANTED) {
            activity.requestPermissions(new String[] {Manifest.permission.POST_NOTIFICATIONS}, 0);
        }
    }

    public static void show(Context context, boolean playing, String text, int noteValue, String playText,
            String stopText) {
        if (sClosed) {
            return;
        }
        Intent intent = new Intent(context, MetronomeService.class)
                .setAction(ACTION_SHOW)
                .putExtra(EXTRA_PLAYING, playing)
                .putExtra(EXTRA_TEXT, text)
                .putExtra(EXTRA_NOTE_VALUE, noteValue)
                .putExtra(EXTRA_PLAY_TEXT, playText)
                .putExtra(EXTRA_STOP_TEXT, stopText);
        ContextCompat.startForegroundService(context, intent);
    }

    public static void hide(Context context) {
        context.stopService(new Intent(context, MetronomeService.class));
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent != null ? intent.getAction() : null;
        if (ACTION_PLAY.equals(action)) {
            nativePlay();
        } else if (ACTION_STOP.equals(action)) {
            nativeStop();
        } else if (ACTION_DISMISS.equals(action) && !mPlaying) {
            ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE);
            stopSelf();
            return START_NOT_STICKY;
        } else if (ACTION_SHOW.equals(action)) {
            mPlaying = intent.getBooleanExtra(EXTRA_PLAYING, false);
            mText = intent.getStringExtra(EXTRA_TEXT);
            mNoteValue = intent.getIntExtra(EXTRA_NOTE_VALUE, 0);
            mPlayText = intent.getStringExtra(EXTRA_PLAY_TEXT);
            mStopText = intent.getStringExtra(EXTRA_STOP_TEXT);
        }
        // Every start must reach startForeground; play and stop update it once the app reports the new state.
        ServiceCompat.startForeground(this, NOTIFICATION_ID, buildNotification(),
                ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
        return START_NOT_STICKY;
    }

    // Closing the app from the recents stops the metronome instead of leaving it playing.
    @Override
    public void onTaskRemoved(Intent rootIntent) {
        sClosed = true;
        nativeStop();
        ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE);
        stopSelf();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private PendingIntent serviceIntent(int requestCode, String action) {
        return PendingIntent.getService(this, requestCode,
                new Intent(this, MetronomeService.class).setAction(action), PendingIntent.FLAG_IMMUTABLE);
    }

    private Notification buildNotification() {
        CharSequence appName = getApplicationInfo().loadLabel(getPackageManager());
        getSystemService(NotificationManager.class).createNotificationChannel(
                new NotificationChannel(CHANNEL_ID, appName, NotificationManager.IMPORTANCE_LOW));

        // Straight to the running QtActivity: the launcher intent would start the splash, and a new Qt activity, again.
        Intent launch = new Intent(this, QtActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent open = PendingIntent.getActivity(this, 0, launch,
                PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);

        return new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(NOTE_ICONS[mNoteValue >= 0 && mNoteValue < NOTE_ICONS.length ? mNoteValue : 0])
                .setContentText(mText)
                .setContentIntent(open)
                .setDeleteIntent(serviceIntent(3, ACTION_DISMISS))
                .addAction(0, mPlaying ? mStopText : mPlayText,
                        mPlaying ? serviceIntent(1, ACTION_STOP) : serviceIntent(2, ACTION_PLAY))
                .setOngoing(mPlaying)
                .setSilent(true)
                .setCategory(NotificationCompat.CATEGORY_SERVICE)
                .setForegroundServiceBehavior(NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE)
                .build();
    }
}
