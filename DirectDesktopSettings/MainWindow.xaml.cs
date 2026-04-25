using System;
using System.Diagnostics;
using System.IO;
using System.Windows;
using Microsoft.Win32;

namespace DirectDesktopSettings
{
    public partial class MainWindow : Window
    {
        private bool _isInitializing = true;
        
        const string BaseRegPath = @"Software\DirectDesktop";
        const string PersRegPath = @"Software\DirectDesktop\Personalize";

        public MainWindow()
        {
            InitializeComponent();
            LoadSettings();
            _isInitializing = false;
            StatusText.Text = "Settings loaded successfully.";
        }

        private void LoadSettings()
        {
            try
            {
                using (var baseKey = Registry.CurrentUser.CreateSubKey(BaseRegPath))
                {
                    ChkTouchView.IsChecked = (int)baseKey.GetValue("TouchView", 0) == 1;
                    ChkTreatDirAsGroup.IsChecked = (int)baseKey.GetValue("TreatDirAsGroup", 0) == 1;
                    ChkFolderItemCount.IsChecked = (int)baseKey.GetValue("FolderItemCount", 1) == 1;
                    
                    int uiMode = (int)baseKey.GetValue("UIMode", 0);
                    if (uiMode == 1) RadQt6.IsChecked = true;
                    else RadDirectUI.IsChecked = true;
                }

                using (var persKey = Registry.CurrentUser.CreateSubKey(PersRegPath))
                {
                    ChkDarkIcons.IsChecked = (int)persKey.GetValue("DarkIcons", 0) == 1;
                    ChkAutoDarkIcons.IsChecked = (int)persKey.GetValue("AutoDarkIcons", 0) == 1;
                    ChkGlassIcons.IsChecked = (int)persKey.GetValue("GlassIcons", 0) == 1;
                }
            }
            catch (Exception ex)
            {
                StatusText.Text = "Error loading settings: " + ex.Message;
            }
        }

        private void Setting_Changed(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            SaveSettings();
        }
        
        private void Mode_Changed(object sender, RoutedEventArgs e)
        {
            if (_isInitializing) return;
            SaveSettings();
        }

        private void SaveSettings()
        {
            try
            {
                using (var baseKey = Registry.CurrentUser.CreateSubKey(BaseRegPath))
                {
                    baseKey.SetValue("TouchView", ChkTouchView.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                    baseKey.SetValue("TreatDirAsGroup", ChkTreatDirAsGroup.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                    baseKey.SetValue("FolderItemCount", ChkFolderItemCount.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                    baseKey.SetValue("UIMode", RadQt6.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                }

                using (var persKey = Registry.CurrentUser.CreateSubKey(PersRegPath))
                {
                    persKey.SetValue("DarkIcons", ChkDarkIcons.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                    persKey.SetValue("AutoDarkIcons", ChkAutoDarkIcons.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                    persKey.SetValue("GlassIcons", ChkGlassIcons.IsChecked == true ? 1 : 0, RegistryValueKind.DWord);
                }
                
                StatusText.Text = "Settings saved successfully at " + DateTime.Now.ToShortTimeString();
            }
            catch (Exception ex)
            {
                StatusText.Text = "Error saving settings: " + ex.Message;
            }
        }

        private void BtnKill_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var processes = Process.GetProcessesByName("DirectDesktop");
                if (processes.Length == 0)
                {
                    StatusText.Text = "DirectDesktop is not currently running.";
                    return;
                }

                foreach (var process in processes)
                {
                    process.Kill();
                    process.WaitForExit(2000); // Give it 2 seconds to die gracefully
                }
                
                // When DD dies, Windows Explorer usually redraws the desktop automatically.
                // Just in case, we can ping Explorer to refresh.
                StatusText.Text = "DirectDesktop forcefully closed. System UI restored.";
            }
            catch (Exception ex)
            {
                StatusText.Text = "Error killing process: " + ex.Message;
            }
        }

        private void BtnStart_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // First kill any existing instances
                var processes = Process.GetProcessesByName("DirectDesktop");
                foreach (var process in processes)
                {
                    process.Kill();
                    process.WaitForExit(1000);
                }

                // Check common build output directories
                string localPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "DirectDesktop.exe");
                string devPathX64Debug = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", "..", "..", "x64", "Debug", "DirectDesktop.exe");
                string devPathX64Release = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", "..", "..", "x64", "Release", "DirectDesktop.exe");
                
                string targetPath = null;
                
                if (File.Exists(localPath)) targetPath = localPath;
                else if (File.Exists(devPathX64Debug)) targetPath = devPathX64Debug;
                else if (File.Exists(devPathX64Release)) targetPath = devPathX64Release;
                
                if (targetPath != null)
                {
                    Process.Start(new ProcessStartInfo
                    {
                        FileName = targetPath,
                        WorkingDirectory = Path.GetDirectoryName(targetPath)
                    });
                    StatusText.Text = "DirectDesktop started: " + targetPath;
                }
                else
                {
                    StatusText.Text = "Could not locate DirectDesktop.exe. Make sure it is compiled in Visual Studio.";
                }
            }
            catch (Exception ex)
            {
                StatusText.Text = "Error starting process: " + ex.Message;
            }
        }
    }
}
