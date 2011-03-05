namespace UAVXGS
{
    partial class Receiver
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Receiver));
            this.ThrottleTrackBar = new System.Windows.Forms.TrackBar();
            this.RollTrackBar = new System.Windows.Forms.TrackBar();
            this.PitchTrackBar = new System.Windows.Forms.TrackBar();
            this.YawTrackBar = new System.Windows.Forms.TrackBar();
            this.RTHTrackBar = new System.Windows.Forms.TrackBar();
            this.ThrottleValue = new System.Windows.Forms.Label();
            this.RollValue = new System.Windows.Forms.Label();
            this.PitchValue = new System.Windows.Forms.Label();
            this.YawValue = new System.Windows.Forms.Label();
            this.RTHValue = new System.Windows.Forms.Label();
            this.ThrottleLabel = new System.Windows.Forms.Label();
            this.RollLabel = new System.Windows.Forms.Label();
            this.PitchLabel = new System.Windows.Forms.Label();
            this.YawLabel = new System.Windows.Forms.Label();
            this.RTHLabel = new System.Windows.Forms.Label();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.okButton = new System.Windows.Forms.Button();
            this.CamPitchTrimTrackBar = new System.Windows.Forms.TrackBar();
            this.NavSTrackBar = new System.Windows.Forms.TrackBar();
            this.CamPitchTrimLabel = new System.Windows.Forms.Label();
            this.NavSLabel = new System.Windows.Forms.Label();
            this.CamPitchTrimValue = new System.Windows.Forms.Label();
            this.NavSValue = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.ThrottleTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.RollTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.PitchTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.YawTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.RTHTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.CamPitchTrimTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.NavSTrackBar)).BeginInit();
            this.SuspendLayout();
            // 
            // ThrottleTrackBar
            // 
            resources.ApplyResources(this.ThrottleTrackBar, "ThrottleTrackBar");
            this.ThrottleTrackBar.LargeChange = 10;
            this.ThrottleTrackBar.Maximum = 125;
            this.ThrottleTrackBar.Minimum = -25;
            this.ThrottleTrackBar.Name = "ThrottleTrackBar";
            this.ThrottleTrackBar.TickFrequency = 10;
            // 
            // RollTrackBar
            // 
            resources.ApplyResources(this.RollTrackBar, "RollTrackBar");
            this.RollTrackBar.LargeChange = 10;
            this.RollTrackBar.Maximum = 125;
            this.RollTrackBar.Minimum = -125;
            this.RollTrackBar.Name = "RollTrackBar";
            this.RollTrackBar.TickFrequency = 10;
            // 
            // PitchTrackBar
            // 
            resources.ApplyResources(this.PitchTrackBar, "PitchTrackBar");
            this.PitchTrackBar.LargeChange = 10;
            this.PitchTrackBar.Maximum = 125;
            this.PitchTrackBar.Minimum = -125;
            this.PitchTrackBar.Name = "PitchTrackBar";
            this.PitchTrackBar.TickFrequency = 10;
            // 
            // YawTrackBar
            // 
            resources.ApplyResources(this.YawTrackBar, "YawTrackBar");
            this.YawTrackBar.LargeChange = 10;
            this.YawTrackBar.Maximum = 125;
            this.YawTrackBar.Minimum = -125;
            this.YawTrackBar.Name = "YawTrackBar";
            this.YawTrackBar.TickFrequency = 10;
            // 
            // RTHTrackBar
            // 
            this.RTHTrackBar.LargeChange = 10;
            resources.ApplyResources(this.RTHTrackBar, "RTHTrackBar");
            this.RTHTrackBar.Maximum = 125;
            this.RTHTrackBar.Minimum = -25;
            this.RTHTrackBar.Name = "RTHTrackBar";
            this.RTHTrackBar.TickFrequency = 10;
            // 
            // ThrottleValue
            // 
            resources.ApplyResources(this.ThrottleValue, "ThrottleValue");
            this.ThrottleValue.Name = "ThrottleValue";
            this.ThrottleValue.TextChanged += new System.EventHandler(this.gasLabel_TextChanged);
            // 
            // RollValue
            // 
            resources.ApplyResources(this.RollValue, "RollValue");
            this.RollValue.Name = "RollValue";
            this.RollValue.TextChanged += new System.EventHandler(this.RollLabel_TextChanged);
            // 
            // PitchValue
            // 
            resources.ApplyResources(this.PitchValue, "PitchValue");
            this.PitchValue.Name = "PitchValue";
            this.PitchValue.TextChanged += new System.EventHandler(this.PitchLabel_TextChanged);
            // 
            // YawValue
            // 
            resources.ApplyResources(this.YawValue, "YawValue");
            this.YawValue.Name = "YawValue";
            this.YawValue.TextChanged += new System.EventHandler(this.YawLabel_TextChanged);
            // 
            // RTHValue
            // 
            resources.ApplyResources(this.RTHValue, "RTHValue");
            this.RTHValue.Name = "RTHValue";
            this.RTHValue.TextChanged += new System.EventHandler(this.ch5Label_TextChanged);
            // 
            // ThrottleLabel
            // 
            resources.ApplyResources(this.ThrottleLabel, "ThrottleLabel");
            this.ThrottleLabel.Name = "ThrottleLabel";
            // 
            // RollLabel
            // 
            resources.ApplyResources(this.RollLabel, "RollLabel");
            this.RollLabel.Name = "RollLabel";
            // 
            // PitchLabel
            // 
            resources.ApplyResources(this.PitchLabel, "PitchLabel");
            this.PitchLabel.Name = "PitchLabel";
            // 
            // YawLabel
            // 
            resources.ApplyResources(this.YawLabel, "YawLabel");
            this.YawLabel.Name = "YawLabel";
            // 
            // RTHLabel
            // 
            resources.ApplyResources(this.RTHLabel, "RTHLabel");
            this.RTHLabel.Name = "RTHLabel";
            // 
            // timer1
            // 
            this.timer1.Interval = 300;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // okButton
            // 
            resources.ApplyResources(this.okButton, "okButton");
            this.okButton.Name = "okButton";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // CamPitchTrimTrackBar
            // 
            resources.ApplyResources(this.CamPitchTrimTrackBar, "CamPitchTrimTrackBar");
            this.CamPitchTrimTrackBar.LargeChange = 10;
            this.CamPitchTrimTrackBar.Maximum = 125;
            this.CamPitchTrimTrackBar.Minimum = -125;
            this.CamPitchTrimTrackBar.Name = "CamPitchTrimTrackBar";
            this.CamPitchTrimTrackBar.TickFrequency = 10;
            // 
            // NavSTrackBar
            // 
            resources.ApplyResources(this.NavSTrackBar, "NavSTrackBar");
            this.NavSTrackBar.LargeChange = 10;
            this.NavSTrackBar.Maximum = 125;
            this.NavSTrackBar.Minimum = -25;
            this.NavSTrackBar.Name = "NavSTrackBar";
            this.NavSTrackBar.TickFrequency = 10;
            // 
            // CamPitchTrimLabel
            // 
            resources.ApplyResources(this.CamPitchTrimLabel, "CamPitchTrimLabel");
            this.CamPitchTrimLabel.Name = "CamPitchTrimLabel";
            // 
            // NavSLabel
            // 
            resources.ApplyResources(this.NavSLabel, "NavSLabel");
            this.NavSLabel.Name = "NavSLabel";
            // 
            // CamPitchTrimValue
            // 
            resources.ApplyResources(this.CamPitchTrimValue, "CamPitchTrimValue");
            this.CamPitchTrimValue.Name = "CamPitchTrimValue";
            this.CamPitchTrimValue.TextChanged += new System.EventHandler(this.ch6Label_TextChanged);
            // 
            // NavSValue
            // 
            resources.ApplyResources(this.NavSValue, "NavSValue");
            this.NavSValue.Name = "NavSValue";
            this.NavSValue.TextChanged += new System.EventHandler(this.ch7Label_TextChanged);
            // 
            // Receiver
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.NavSValue);
            this.Controls.Add(this.CamPitchTrimValue);
            this.Controls.Add(this.NavSLabel);
            this.Controls.Add(this.CamPitchTrimLabel);
            this.Controls.Add(this.NavSTrackBar);
            this.Controls.Add(this.CamPitchTrimTrackBar);
            this.Controls.Add(this.okButton);
            this.Controls.Add(this.RTHLabel);
            this.Controls.Add(this.YawLabel);
            this.Controls.Add(this.PitchLabel);
            this.Controls.Add(this.RollLabel);
            this.Controls.Add(this.ThrottleLabel);
            this.Controls.Add(this.RTHValue);
            this.Controls.Add(this.YawValue);
            this.Controls.Add(this.PitchValue);
            this.Controls.Add(this.RollValue);
            this.Controls.Add(this.ThrottleValue);
            this.Controls.Add(this.RTHTrackBar);
            this.Controls.Add(this.YawTrackBar);
            this.Controls.Add(this.PitchTrackBar);
            this.Controls.Add(this.RollTrackBar);
            this.Controls.Add(this.ThrottleTrackBar);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "Receiver";
            this.ShowInTaskbar = false;
            this.VisibleChanged += new System.EventHandler(this.Receiver_VisibleChanged);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Receiver_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.ThrottleTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.RollTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.PitchTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.YawTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.RTHTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.CamPitchTrimTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.NavSTrackBar)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TrackBar ThrottleTrackBar;
        private System.Windows.Forms.TrackBar RollTrackBar;
        private System.Windows.Forms.TrackBar PitchTrackBar;
        private System.Windows.Forms.TrackBar YawTrackBar;
        private System.Windows.Forms.TrackBar RTHTrackBar;
        private System.Windows.Forms.Label ThrottleLabel;
        private System.Windows.Forms.Label RollLabel;
        private System.Windows.Forms.Label PitchLabel;
        private System.Windows.Forms.Label YawLabel;
        private System.Windows.Forms.Label RTHLabel;
        public System.Windows.Forms.Label ThrottleValue;
        public System.Windows.Forms.Label RollValue;
        public System.Windows.Forms.Label PitchValue;
        public System.Windows.Forms.Label YawValue;
        public System.Windows.Forms.Label RTHValue;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.Button okButton;
        private System.Windows.Forms.TrackBar CamPitchTrimTrackBar;
        private System.Windows.Forms.TrackBar NavSTrackBar;
        private System.Windows.Forms.Label CamPitchTrimLabel;
        private System.Windows.Forms.Label NavSLabel;
        public System.Windows.Forms.Label CamPitchTrimValue;
        public System.Windows.Forms.Label NavSValue;

    }
}