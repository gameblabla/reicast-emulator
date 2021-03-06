package com.reicast.emulator.config;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.constraint.ConstraintLayout;
import android.support.design.widget.Snackbar;
import android.support.graphics.drawable.VectorDrawableCompat;
import android.support.v4.app.Fragment;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Spinner;
import android.widget.TextView;

import com.android.util.FileUtils;
import com.reicast.emulator.Emulator;
import com.reicast.emulator.R;

import java.io.File;
import java.io.FilenameFilter;
import java.lang.ref.WeakReference;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PGConfigFragment extends Fragment {

	private Spinner mSpnrConfigs;

	private CompoundButton unstable_opt;
	private CompoundButton safemode_opt;
	private EditText mainFrames;
	private SeekBar frameSeek;
	private CompoundButton pvr_render;
	private CompoundButton synced_render;
	private CompoundButton queue_render;
	private CompoundButton modifier_volumes;
	private CompoundButton interrupt_opt;

	@Override
	public void onAttach(Activity activity) {
		super.onAttach(activity);
	}

	@Override
	public void onAttach(Context context) {
		super.onAttach(context);
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
							 Bundle savedInstanceState) {
		// Inflate the layout for this fragment
		return inflater.inflate(R.layout.pgconfig_fragment, container, false);
	}

	@Override
	public void onViewCreated(View view, Bundle savedInstanceState) {

		Emulator app = (Emulator) getActivity().getApplicationContext();
		app.getConfigurationPrefs(PreferenceManager.getDefaultSharedPreferences(getActivity()));

		mSpnrConfigs = (Spinner) getView().findViewById(R.id.config_spinner);
		new LocateConfigs(PGConfigFragment.this).execute("/data/data/"
				+ getActivity().getPackageName() + "/shared_prefs/");

		unstable_opt = (CompoundButton) getView().findViewById(R.id.unstable_option);
		safemode_opt = (CompoundButton) getView().findViewById(R.id.dynarec_safemode);
		mainFrames = (EditText) getView().findViewById(R.id.current_frames);
		frameSeek = (SeekBar) getView().findViewById(R.id.frame_seekbar);
		pvr_render = (CompoundButton) getView().findViewById(R.id.render_option);
		synced_render = (CompoundButton) getView().findViewById(R.id.syncrender_option);
		queue_render = (CompoundButton) getView().findViewById(R.id.queuerender_option);
		modifier_volumes = (CompoundButton) getView().findViewById(R.id.modvols_option);
		interrupt_opt = (CompoundButton) getView().findViewById(R.id.interrupt_option);
	}

	private void saveSettings(SharedPreferences mPrefs) {
		mPrefs.edit().putBoolean(Emulator.pref_unstable, unstable_opt.isChecked())
				.putBoolean(Emulator.pref_dynsafemode, safemode_opt.isChecked())
				.putInt(Emulator.pref_frameskip, frameSeek.getProgress())
				.putBoolean(Emulator.pref_pvrrender, pvr_render.isChecked())
				.putBoolean(Emulator.pref_syncedrender, synced_render.isChecked())
				.putBoolean(Emulator.pref_queuerender, queue_render.isChecked())
				.putBoolean(Emulator.pref_modvols, modifier_volumes.isChecked())
				.putBoolean(Emulator.pref_interrupt, interrupt_opt.isChecked()).apply();
		showToastMessage(getActivity().getString(R.string.pgconfig_saved),
				Snackbar.LENGTH_SHORT);
	}

	private void configureViewByGame(String gameId) {
		final SharedPreferences mPrefs = getActivity()
				.getSharedPreferences(gameId, Activity.MODE_PRIVATE);
		unstable_opt.setChecked(mPrefs.getBoolean(Emulator.pref_unstable, Emulator.unstableopt));
		safemode_opt.setChecked(mPrefs.getBoolean(Emulator.pref_dynsafemode, Emulator.dynsafemode));

		int frameskip = mPrefs.getInt(Emulator.pref_frameskip, Emulator.frameskip);
		mainFrames.setText(String.valueOf(frameskip));

		frameSeek.setProgress(frameskip);
		frameSeek.setIndeterminate(false);
		frameSeek.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
				mainFrames.setText(String.valueOf(progress));
			}

			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}

			public void onStopTrackingTouch(SeekBar seekBar) {

			}
		});
		mainFrames.addTextChangedListener(new TextWatcher() {
			public void afterTextChanged(Editable s) {
				Editable frameText = mainFrames.getText();
				if (frameText != null) {
					int frames = Integer.parseInt(frameText.toString());
					frameSeek.setProgress(frames);
				}
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) {
			}

			public void onTextChanged(CharSequence s, int start, int before, int count) {
			}
		});

		pvr_render.setChecked(mPrefs.getBoolean(Emulator.pref_pvrrender, Emulator.pvrrender));
		synced_render.setChecked(mPrefs.getBoolean(Emulator.pref_syncedrender, Emulator.syncedrender));
		queue_render.setChecked(mPrefs.getBoolean(Emulator.pref_queuerender, Emulator.queuerender));
		modifier_volumes.setChecked(mPrefs.getBoolean(Emulator.pref_modvols, Emulator.modvols));
		interrupt_opt.setChecked(mPrefs.getBoolean(Emulator.pref_interrupt, Emulator.interrupt));

		Button savePGC = (Button) getView().findViewById(R.id.save_pg_btn);
		savePGC.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				saveSettings(mPrefs);
			}
		});
	}

	private static class LocateConfigs extends AsyncTask<String, Integer, List<File>> {
		private WeakReference<PGConfigFragment> options;

		LocateConfigs(PGConfigFragment context) {
			options = new WeakReference<>(context);
		}

		@Override
		protected List<File> doInBackground(String... paths) {
			File storage = new File(paths[0]);
			Log.d("Files", storage.getAbsolutePath());
			FilenameFilter[] filter = new FilenameFilter[1];
			filter[0] = new FilenameFilter() {
				public boolean accept(File dir, String name) {
					return !name.endsWith("_preferences.xml");
				}
			};
			FileUtils fileUtils = new FileUtils();
			Collection<File> files = fileUtils.listFiles(storage, filter, 0);
			return (List<File>) files;
		}

		@Override
		protected void onPostExecute(List<File> items) {
			if (items != null && !items.isEmpty()) {
				final Map<String, String> gameMap = new HashMap<>();
				String[] titles = new String[items.size()];
				for (int i = 0; i < items.size(); i ++) {
					String filename = items.get(i).getName();
					String gameFile = filename.substring(0, filename.length() - 4);
					SharedPreferences mPrefs = options.get().getActivity()
							.getSharedPreferences(gameFile, Activity.MODE_PRIVATE);
					titles[i] = mPrefs.getString(Config.game_title, "Title Unavailable");
					gameMap.put(titles[i], gameFile);
				}
				ArrayAdapter<String> configAdapter = new ArrayAdapter<String>(
						options.get().getActivity(), android.R.layout.simple_spinner_item, titles);
				configAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
				options.get().mSpnrConfigs.setAdapter(configAdapter);
				options.get().mSpnrConfigs.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
					@Override
					public void onItemSelected(AdapterView<?> parent, View select, int pos, long id) {
						options.get().configureViewByGame(gameMap.get(
								String.valueOf(parent.getItemAtPosition(pos))
						));
					}
					@Override
					public void onNothingSelected(AdapterView<?> parentView) {

					}
				});
			} else {
				options.get().mSpnrConfigs.setEnabled(false);
			}
		}
	}

	private void showToastMessage(String message, int duration) {
		ConstraintLayout layout = (ConstraintLayout) getActivity().findViewById(R.id.mainui_layout);
		Snackbar snackbar = Snackbar.make(layout, message, duration);
		View snackbarLayout = snackbar.getView();
		TextView textView = (TextView) snackbarLayout.findViewById(
				android.support.design.R.id.snackbar_text);
		textView.setGravity(Gravity.CENTER_VERTICAL);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
			textView.setTextAlignment(View.TEXT_ALIGNMENT_GRAVITY);
		Drawable drawable;
		if (android.os.Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
			drawable = getResources().getDrawable(
					R.drawable.ic_settings, getActivity().getTheme());
		} else {
			drawable = VectorDrawableCompat.create(getResources(),
					R.drawable.ic_settings, getActivity().getTheme());
		}
		textView.setCompoundDrawablesWithIntrinsicBounds(drawable, null, null, null);
		textView.setCompoundDrawablePadding(getResources()
				.getDimensionPixelOffset(R.dimen.snackbar_icon_padding));
		snackbar.show();
	}
}
