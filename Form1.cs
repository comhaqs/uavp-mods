// =================================================================================================
// =                                  UAVX Quadrocopter Groundstation                                 =
// =                             Copyright (c) 2008 by Prof. Greg Egan                             =
// =                         Instruments Copyright (c) 2008 Guillaume Choutea                      =                                        */
// =                               http://code.google.com/p/uavp-mods/                             =
// =================================================================================================

//    This is part of UAVX.

//    UAVX is free software: you can redistribute it and/or modify it under the terms of the GNU 
//    General Public License as published by the Free Software Foundation, either version 3 of the 
//    License, or (at your option) any later version.

//    UAVX is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without even 
//    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//    General Public License for more details.

//    You should have received a copy of the GNU General Public License along with this program.  
//    If not, see http://www.gnu.org/licenses/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.IO;


namespace UAVXGS
{
    public partial class FormMain : Form
    {
        // ASCII Constants
        const byte NUL = 0;
        const byte SOH = 1;
        const byte EOT = 4;
        const byte ACK = 6;
        const byte HT = 9;
        const byte LF = 10;
        const byte CR = 13;
        const byte NAK = 21;
        const byte ESC = 27;

       // const int DefaultBaudRate = 9600;

        const Byte RCMaximum = 238;
        const double OUTMaximumScale = 0.5; // 100/200 % for PWM at least

        const byte UnknownPacketTag = 0;
        const byte RestartPacketTag = 8;
        const byte UAVXFlightPacketTag = 13;
        const byte UAVXNavPacketTag = 14;
        const byte UAVXStatsPacketTag = 15;
        const byte UAVXControlPacketTag = 16;
        const byte UAVXParamsPacketTag = 17;
        const byte UAVXMinPacketTag = 18;
        const byte FrSkyPacketTag = 99;

        const byte WaitRxSentinel = 0;
        const byte WaitRxBody = 1;
        const byte WaitRxESC = 2;
        const byte WaitRxCheck = 3;
        const byte WaitRxCopy = 4;
        const byte WaitUPTag = 5;
        const byte WaitUPLength = 6;
        const byte WaitUPBody = 7;
        const byte WaitRxTag = 8;

        const byte MAXPARAMS = 64;

        const float MILLIRADDEG = (float)0.057295;
        const byte AttitudeToDegrees = 28;

        const int DefaultRangeLimit = 100;
        const int MaximumRangeLimit = 250; // You carry total responsibility if you increase this value
        const int MaximumAltitudeLimit = 121; // You carry total responsibility if you increase this value 

        const double CurrentSensorMax = 50.0; // depends on current ADC used - needs calibration box?
        const double YawGyroRate = 400.0;
        const double RollPitchGyroRate = 400.0;

        /*
        UAVXStatsPacket      
        */

        short I2CFailsT;
        short GPSFailsT; 
        short AccFailsT; 
        short GyroFailsT; 
        short CompassFailsT; 
        short BaroFailsT; 
        short I2CESCFailsT;
        short RCFailSafesT;

        short GPSAltitudeT;
        short GPSMaxVelT;
        short GPSMinSatsT;
        short GPSMaxSatsT;
        short GPSMinHDiluteT;
        short GPSMaxHDiluteT;
        
        short BaroRelAltitudeT;
        short BaroMinROCT;
        short BaroMaxROCT;

        short MinTempT;
        short MaxTempT;

        short BadT;
        short AirframeT;
        short OrientT;
        short BadNumT;

        /*
        UAVXFlightPacket
        
        Flags bit values
        Flags[0]
            AltHoldEnabled	// stick programmed	
            TurnToWP	    // stick programmed		
            GyroFailure
            LostModel
            NearLevel
            LowBatt
            GPSValid
            NavValid

        Flags[1]
            BaroFailure
            AccFailure
            CompassFailure
            GPSFailure
            AttitudeHold
            ThrottleMoving
            Hovering
            Navigate
                
        Flags[2]
            ReturnHome
            Proximity
            CloseProximity
            UsingGPSAlt
            UsingRTHAutoDescend
            BaroAltitudeValid
            RangefinderAltitudeValid
            UsingRangefinderAlt
         
        Flags[3]
		    AllowNavAltitudeHold	// stick programmed
		    UsingPositionHoldLock
		    Ch5Active
		    Simulation
		    AcquireNewPosition 
		    MotorsArmed
			NavigationActive
		    ForceFailsafe

         Flags[4]
            Signal
		    RCFrameOK
		    ParametersValid
		    RCNewValues
		    NewCommands
		    AccelerationsValid
		    CompassValid
		    CompassMissRead

        Flags[5]
		    UsingPolarCoordinates
		    ReceivingGPS
		    PacketReceived
		    NavComputed
		    AltitudeValid		
		    UsingSerialPPM
		    UsingTxMode2
		    Unused0
		    
         */

        // byte UAVXFlightPacketTag;   
        // byte Length;  
        const byte NoOfFlagBytes = 6;
        const byte NoOfOutputs = 8;
        byte[] Flags = new byte[NoOfFlagBytes];
        byte StateT;                    // 8
        short BatteryVoltsT;            // 9
        short BatteryCurrentT;          // 11
 		short BatteryChargeT;           // 13
        short RCGlitchesT;              // 15
        short DesiredThrottleT;         // 17
        short DesiredRollT;             // 19
        short DesiredPitchT;            // 21
        short DesiredYawT;              // 23
        short RollRateT;                // 25
        short PitchRateT;               // 27
        short YawRateT;                 // 29
        short RollAngleT;                 // 31
        short PitchAngleT;                // 33
        short YawAngleT;                  // 35
        short LRAccT;                   // 37
        short FBAccT;                   // 39
        short DUAccT;                   // 41
        short LRCompT;                  // 43
        short FBCompT;                  // 44
        short DUCompT;                  // 45
        short AltCompT;                 // 46
        short[] OutputT = new short[8];   // 47
        long MissionTimeMilliSecT;       // 53

        /*
        UAVXMinPacketTag;   
        Length;  
 
        Flags[]

        State                           // 8
        NavState                        // etc.
        FailState
        
        BatteryVolts
        BatteryCurrent
        BatteryChargeUsedAH

        RollAngle
        PitchAngle

        BaroRelAltitude
        RangefinderAltitude        

        Heading

        GPSLatitude
        GPSLongitude

        UAVXAirframet
        Orientation

        MissionTimeMilliSecT
        */

        // UAVXNavPacket
        //byte UAVXNavPacketTag;
        //byte Length;
        byte NavStateT;                 // 2
        byte FailStateT;                // 3
        byte GPSNoOfSatsT;              // 4
        byte GPSFixT;                   // 5

        byte CurrWPT;                   // 6

        short BaroROCT;                 // 7 
        int RelBaroAltitudeT;           // 9

        short GPSHeadingT;              // 12
        short RangefinderAltitudeT;     // 14

        short GPSHDiluteT;              // 16
        short HeadingT;                 // 18
        short DesiredCourseT;           // 20

        short GPSVelT;                  // 22
        short GPSROCT;                  // 24
        int GPSRelAltitudeT;            // 26
        int GPSLatitudeT;               // 29
        int GPSLongitudeT;              // 33
        int DesiredAltitudeT;           // 37
        int DesiredLatitudeT;           // 40
        int DesiredLongitudeT;          // 44
        int NavStateTimeoutT;           // 48
        short AmbientTempT;             // 51
        int GPSMissionTimeT;            // 53

        short NavSensitivityT;
        short NavRCorrT;
        short NavPCorrT;
        short NavYCorrT;

        double Distance, LongitudeCorrection, WhereDirection;
        bool FirstGPSCoordinates = true;
        int OriginLatitude, OriginLongitude;
        double EastDiff, NorthDiff;
        int FlightHeading;
        double FlightHeadingOffset = 0.0;
        double FlightRoll, FlightPitch;
        double FlightRollp = 0.0;
        double FlightPitchp = 0.0;
        double OSO = 0.0;
        double OCO = 1.0;
        bool DoneOrientation = false;

        bool UAVXArm = false;

        byte[] UAVXPacket = new byte[256];
        byte[] FrSkyPacket = new byte[16];

        const short RxQueueLength = 2048;
        const short RxQueueMask = RxQueueLength - 1;
        byte[] RxQueue = new byte[RxQueueLength];
        short RxTail = 0;
        short RxHead = 0;
        long NavPacketsReceived = 0;
        long FlightPacketsReceived = 0;
        long StatsPacketsReceived = 0;
        long ControlPacketsReceived = 0;
        long FrSkyPacketsReceived = 0;

        short ParamSet, ParamNo;

        short FrSkyRxPacketByteCount;
        short FrSkyRxPacketTag, FrSkyReceivedPacketTag, FrSkyPacketRxState;
        bool FrSkyPacketReceived = false;

        short RxPacketByteCount;
        byte RxPacketTag, RxPacketLength, PacketRxState;
        byte ReceivedPacketTag;
        bool PacketReceived = false;
        long ReplayProgress = 0;
        int ReplayDelay = 1;

        bool CheckSumError;
        short RxLengthErrors = 0, RxCheckSumErrors = 0, RxIllegalErrors = 0, RxFrSkyErrors = 0;

        double[,] Params = new double[2,MAXPARAMS];

        byte RxCheckSum;

      //  int COMBaudRate = DefaultBaudRate;

        double CurrAlt, AltError;

        bool InFlight = false;

        System.IO.FileStream SaveLogFileStream;
        System.IO.BinaryWriter SaveLogFileBinaryWriter; 
        System.IO.FileStream SaveTextLogFileStream;
        System.IO.StreamWriter SaveTextLogFileStreamWriter;
        System.IO.FileStream SaveTextParamFileStream;
        System.IO.StreamWriter SaveTextParamFileStreamWriter;

        System.IO.FileStream OpenLogFileStream;
        System.IO.BinaryReader OpenLogFileBinaryReader; 
        bool DoingLogfileReplay = false;
        bool ReadingTelemetry = false;

        public FormMain()
        {
          InitializeComponent();

         // COMSelectComboBox.Items.Add("Select COM Port");
          string[] AvailableCOMPorts = ComPorts.readPorts();
          foreach (string AvailableCOMPort in AvailableCOMPorts)
              COMSelectComboBox.Items.Add(AvailableCOMPort);
          COMSelectComboBox.Text = UAVXGS.Properties.Settings.Default.COMPort;
          COMBaudRateComboBox.Text = string.Format("{0:n0}", UAVXGS.Properties.Settings.Default.COMBaudRate);
          // Assign assembly version number to title bar text
          Version vrs = new Version(Application.ProductVersion);
          this.Text = this.Text + " v" + vrs.Major + "." + vrs.Minor + "." + vrs.Build;// vrs.MajorRevision + 

          ReplayDelay = 20 - Convert.ToInt16(ReplayNumericUpDown.Text);

         // FrSkycheckBox1.Checked = UAVXGS.Properties.Settings.Default.IsFrSky;
       
          // Create default data dir if it doesn't exist
          string AppLogDir = UAVXGS.Properties.Settings.Default.LogFileDirectory;
          if (!(Directory.Exists(AppLogDir))) {
            string sProgFilesLogDir = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
            sProgFilesLogDir += "\\" + "UAVXGS " + vrs.Major + "." + vrs.Minor;
            if (!(Directory.Exists(sProgFilesLogDir))) {
              Directory.CreateDirectory(sProgFilesLogDir);
            }
            UAVXGS.Properties.Settings.Default.LogFileDirectory = sProgFilesLogDir;

          } 
        }
/*
        public FormMain()
        {
            InitializeComponent();

            // COMSelectComboBox.Items.Add("Select COM Port");
            string[] AvailableCOMPorts = ComPorts.readPorts();
            foreach (string AvailableCOMPort in AvailableCOMPorts)
                COMSelectComboBox.Items.Add(AvailableCOMPort);
            COMSelectComboBox.Text = UAVXGS.Properties.Settings.Default.COMPort;
            COMBaudRateComboBox.Text = string.Format("{0:n0}", UAVXGS.Properties.Settings.Default.COMBaudRate);

            Version vrs = new Version(Application.ProductVersion);
            this.Text = this.Text + " v" + vrs.Major + "." + vrs.Minor + "." + vrs.Build;

            ReplayDelay = 20 - Convert.ToInt16(ReplayNumericUpDown.Text);
        }
*/
        public static class ComPorts
        {
            public static string[] readPorts()
            {
                string[] ComPorts = System.IO.Ports.SerialPort.GetPortNames();
                Array.Sort(ComPorts);
                return ComPorts;
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                e.Cancel = true; //cancel the fom closing
                Thread CloseDown = new Thread(new ThreadStart(CloseSerialOnExit)); //close port in new thread to avoid hang
                CloseDown.Start(); //close port in new thread to avoid hang
            }
        }

        private void CloseSerialOnExit()
        {
            try
            {
                serialPort1.Close(); //close the serial port
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message); //catch any serial port closing error messages
            }
            this.Invoke(new EventHandler(NowClose)); //now close back in the main thread
        }

        private void NowClose(object sender, EventArgs e)
        {
            this.Close(); //now close the form
        }

        public void SetSerialPort(string portSelected, int speed, ref string errorMessage)
        {
            try
            {
                errorMessage = "";
                serialPort1.PortName = portSelected;
                serialPort1.ReadBufferSize = 8192;
                serialPort1.BaudRate = speed;
                serialPort1.ReceivedBytesThreshold = 1;
                serialPort1.Open();
                serialPort1.Close();
            }
            catch (Exception er)
            {
                //MessageBox.Show(Convert.ToString(er.Message));
                errorMessage = Convert.ToString(er.Message);
            }
        }


       // private void Form1_FormClosing(object sender, FormClosingEventArgs e)
       // {
       //     if (serialPort1.IsOpen) serialPort1.Close();
       // }

        private void COMSelectComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            UAVXGS.Properties.Settings.Default.COMPort = COMSelectComboBox.Text;
            UAVXGS.Properties.Settings.Default.Save();
        }

        private void COMBaudRateComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            UAVXGS.Properties.Settings.Default.COMBaudRate = Convert.ToInt32(COMBaudRateComboBox.Text);
            UAVXGS.Properties.Settings.Default.Save();
        }

        private void textBox1_KeyPress(object sender, KeyPressEventArgs e)
        {
            // If the port is closed, don't try to send a character.
            if (!serialPort1.IsOpen) return;

            // If the port is Open, declare a char[] array with one element.
            char[] buff = new char[1];

            // Load element 0 with the key character.
            buff[0] = e.KeyChar;

            // Send the one character buffer.
            serialPort1.Write(buff, 0, 1);

            // Set the KeyPress event as handled so the character won't
            // display locally. If you want it to display, omit the next line.
            e.Handled = true;
        }

        private void ReplayButton_Click(object sender, EventArgs e)
        {
            if (!DoingLogfileReplay)
            {
                OpenLogFileDialog.Filter = "UAVX Log File (*.log)|*.log";
                OpenLogFileDialog.InitialDirectory = UAVXGS.Properties.Settings.Default.LogFileDirectory;
      
                if (OpenLogFileDialog.ShowDialog() == DialogResult.OK)
                {
                    UAVXGS.Properties.Settings.Default.LogFileDirectory = OpenLogFileDialog.InitialDirectory;
                    OpenLogFileStream = new System.IO.FileStream(OpenLogFileDialog.FileName, System.IO.FileMode.Open);
                    OpenLogFileBinaryReader = new System.IO.BinaryReader(OpenLogFileStream);

                    UAVXGS.Properties.Settings.Default.LogFileDirectory = System.IO.Directory.GetCurrentDirectory();
                    
                    SaveTextLogFileStream = new System.IO.FileStream("UAVX_Replay.csv", System.IO.FileMode.Create);
                    SaveTextLogFileStreamWriter = new System.IO.StreamWriter(SaveTextLogFileStream, System.Text.Encoding.ASCII);
                    WriteTextLogFileHeader();
                    DoingLogfileReplay = true;

                    SaveTextParamFileStream = new System.IO.FileStream("UAVX_Param.csv", System.IO.FileMode.Create);
                    SaveTextParamFileStreamWriter = new System.IO.StreamWriter(SaveTextParamFileStream, System.Text.Encoding.ASCII);
                    WriteTextParamFileHeader();

                    ReplayProgressBar.Value = 0;
                    ReplayProgress = 0;
                    Thread Replay = new Thread(new ThreadStart(ReadReplayLogFile)); 
                    Replay.Start(); 
                }
            }
            else
                CloseReplayLogFile();
        }

        private void CloseReplayLogFile()
        {
            OpenLogFileStream.Close();
            SaveTextLogFileStreamWriter.Flush();
            SaveTextLogFileStreamWriter.Close();
            SaveTextLogFileStream.Close();
            SaveTextParamFileStreamWriter.Flush();
            SaveTextParamFileStreamWriter.Close();
            SaveTextParamFileStream.Close();
        }

        private void CreateSaveLogFile()
        {
            string FileName;

            FileName = UAVXGS.Properties.Settings.Default.LogFileDirectory + "\\UAVX_" + 
                DateTime.Now.Year + "_" +
                DateTime.Now.Month + "_" +
                DateTime.Now.Day + "_" +
                DateTime.Now.Hour + "_" +
                DateTime.Now.Minute;

            SaveLogFileStream = new System.IO.FileStream(FileName + ".log", System.IO.FileMode.Create);
            SaveLogFileBinaryWriter = new System.IO.BinaryWriter(SaveLogFileStream); 

            SaveTextLogFileStream = new System.IO.FileStream(FileName + ".csv", System.IO.FileMode.Create);
            SaveTextLogFileStreamWriter = new System.IO.StreamWriter(SaveTextLogFileStream, System.Text.Encoding.ASCII);
            WriteTextLogFileHeader();

            SaveTextParamFileStream = new System.IO.FileStream(FileName + "_Param.csv", System.IO.FileMode.Create);
            SaveTextParamFileStreamWriter = new System.IO.StreamWriter(SaveTextParamFileStream, System.Text.Encoding.ASCII);
            WriteTextParamFileHeader();

            NavPacketsReceived = 0;
            FlightPacketsReceived = 0;
            StatsPacketsReceived = 0;
        }

        private void WriteTextParamFileHeader()
        {
            SaveTextParamFileStreamWriter.WriteLine("Param," + "#," +
                "RollKp," +
                "RollKi," +
                "RollKd," +
                "HorizDampKp," +
                "RollIntLimit," +
                "PitchKp," +
                "PitchKi," +
                "PitchKd," +
                "AltKp," +
                "PitchIntLimit," +  

                "YawKp," +
                "YawKi," +
                "YawKd," +
                "YawLimit," +
                "YawIntLimit," +
                "ConfigBits," +
                "TimeSlots," +
                "LowVoltThres," +
                "CamRollKp," +
                "PercentCruiseThr," +

                "VertDampKp," +
                "MiddleDU," +
                "PercentIdleThr," +
                "MiddleLR," +
                "MiddleFB," +
                "CamPitchKp," +
                "CompassKp," +
                "AltKi," +
                "was NavRadius," +
                "NavKi," +

                "GSThrottle," +
                "Acro," +
                "NavRTHAlt," +
                "NavMagVar," +
                "GyroType," +
                "ESCType," +
                "TxRxType," +
                "NeutralRadius," +
                "PercentNavSens6Ch," +
                "CamRollTrim," +

                "NavKd," +
                "VertDampDecay," +
                "HorizDampDecay," +
                "BaroScale," +
                "TelemetryType," +
                "MaxDescentRateDmpS," +
                "DescentDelayS," +
                "NavIntLimit," +
                "AltIntLimit," +
                "Unused50," +

                "Unused51," +
                "ServoSense," + 
                "CompassOffsetQtr," +
                "BatteryCapacity," +
                "GyroYawType," +     
                "AltKd," +
                "Orient," +   
                "NavYawLimit," +
                "Balance");


        }

        private void WriteTextLogFileHeader()
        {
            SaveTextLogFileStreamWriter.Write("Flight," +
                "AltHEn," +
                "TTPosEn," + // stick programmed
                "GyroFail," +
                "Lost," +
                "Level," +
                "LowBatt," +
                "GPSVal," +
                "NavVal," +

                "BaroFail," +
                "AccFail," +
                "CompFail," +
                "GPSFail," +
                "AttH," +
                "ThrMov," +
                "AltH," +
                "Nav," +

                "RTH," +
                "Prox," +
                "CloseProx," +
                "UseGPSAlt," +
                "UseRTHDes," +
                "BaroVal," +
                "RFVal," +
                "UseRFAlt," +

                "NavAltHEn," +   // stick programmed
                "POFEn," +
                "POF," +
                "Sim," +
                "AcqPos," +
                "Armed," +
                "NavActive," +
                "StkFreeze," +

                "Signal," +
		        "RCOK," +
		        "ParamVal," +
		        "RCNew," +
		        "NewComm," +
		        "AccVal," +
		        "CompVal," +
		        "CompMissRd," +

		        "UsePolar," +
		        "RxGPS," +
		        "PktRx," +
		        "NavComp," +
		        "AltVal," +		
		        "UseSerPPM," +
		        "UseMode2," +
                "Unused0,");

                SaveTextLogFileStreamWriter.Write("StateT," +
                "BattV," +
                "BattI," +
                "BattCh," +
                "RCGlitches," +
                "DesThr," +
                "DesRoll," +
                "DesPitch," +
                "DesYaw," +
                "RollRate," +
                "PitchRate," +
                "YawRate," +
                "RollSum," +
                "PitchSum," +
                "YawSum," +
                "LRAcc," +
                "FBAcc," +
                "DUAcc," +
                "LRComp," +
                "FBComp," +
                "DUComp," +
                "AltComp,");

                SaveTextLogFileStreamWriter.Write("M1,M2,M3,M4,M5,M6,CamP,CamR,"); 

                SaveTextLogFileStreamWriter.Write("Nav," +
                "NavState," +
                "FailState," +
                "GPSSats," +
                "GPSFix," +
                "CurrWP," +
                "BaroROC," +
                "RelBaroAlt," +
                "Unused," +
                "RFAlt," +
                "GPSHD," +
                "Heading," +
                "DesCourse," +
                "GPSVel," +
                "Unused," +
                "GPSRelAlt," +
                "GPSLat," +
                "GPSLon," +
                "DesAlt," +
                "DesLat," +
                "DesLon," +
                "NavStateTimeout," +
                "AmbTemp," +
                "GPSTime," +
                "Sens," +
                "RCorr," +
                "PCorr," +
                "YCorr,");

                SaveTextLogFileStreamWriter.WriteLine("Stats," +
                "I2CFails," +
                "GPSFails," +
                "AccFails," +
                "GyroFails," +
                "CompFails," +
                "BaroFails," +
                "I2CESCFails," +
                "RCFSafes," +
                "GPSAlt," +
                "GPSMaxVel," +
                "GPSMinSats," +
                "GPSMaxSats," +
                "GPSMinHD," +
                "GPSMaxHD," +
                "BaroRelAlt," +
                "BaroMinROC," +
                "BaroMaxROC," +
                "MinTemp," +
                "MaxTemp," +
                "BadReferGKE," +
                "AFType, Orient, BadNum");
        }  

        private void FlyingButton_Click(object sender, EventArgs e)
        {
            if (InFlight)
            {
                InFlight = false;
                UAVXCloseTelemetry();
                FlyingButton.Text = "Disconnected";
                FlyingButton.BackColor = System.Drawing.Color.Red;
            }
            else
            {
                FlyingButton.Text = "Connected";
                FlyingButton.BackColor = System.Drawing.Color.Green;
                UAVXOpenTelemetry();
            }
 
         //   e.Handled = true;
        }

        //-----------------------------------------------------------------------
        // UAVX Communications

        private void UAVXOpenTelemetry()
        {
            string sError = "";

            UAVXCloseTelemetry();
            
            SetSerialPort(UAVXGS.Properties.Settings.Default.COMPort, UAVXGS.Properties.Settings.Default.COMBaudRate, ref sError);
            if (sError != "")
            {
                MessageTextBox.Text = sError;
                InFlight = false;
            }
            else
            {
                CreateSaveLogFile();
                RxHead = RxTail = 0;
                InitPollPacket();
                serialPort1.Open();
                if (serialPort1.IsOpen)
                    InFlight = true;
                else
                {
                    UAVXCloseTelemetry();
                    InFlight = false;
                }
            } 
        }

        private void ReadReplayLogFile()
        {
            byte b;

            while ( OpenLogFileStream.Position < OpenLogFileStream.Length )
            {
                RxTail++;
                RxTail &= RxQueueMask;
                b = OpenLogFileBinaryReader.ReadByte();
                RxQueue[RxTail] = b;

                ReadingTelemetry = true;
                this.Invoke(new EventHandler(UAVXReadTelemetry));
                while (ReadingTelemetry) { };

                ReplayProgress = (int) (100 * OpenLogFileStream.Position) / OpenLogFileStream.Length;

                Thread.Sleep(ReplayDelay);
            }

            if (OpenLogFileStream.Position == OpenLogFileStream.Length)
            {
                DoingLogfileReplay = false;
                CloseReplayLogFile();
            }
             //this.Invoke(new EventHandler(NowCloseReplay)); //now close back in the main thread
        }

        private void NowCloseReplay(object sender, EventArgs e)
        {
            this.Close(); 
        }

        private void serialPort1_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            byte b;
            short NewRxTail;

            if (serialPort1.IsOpen) // flakey serial drivers hence heroics
            {
                while ( serialPort1.IsOpen && (serialPort1.BytesToRead != 0))
                {
                    NewRxTail = RxTail;
                    NewRxTail++;
                    NewRxTail &= RxQueueMask;
                    if (NewRxTail != RxHead)
                    {
                        RxTail = NewRxTail;
                        b = (byte)serialPort1.ReadByte();
                        RxQueue[RxTail] = b;
                        SaveLogFileBinaryWriter.Write(b);
                    }
                    else
                    {
                        b = (byte)serialPort1.ReadByte(); // processing overflow - discard
                        SaveLogFileBinaryWriter.Write(b);
                    }
                }
                if (serialPort1.IsOpen) 
                    this.Invoke(new EventHandler(UAVXReadTelemetry));
            }
        }

        void InitPollPacket()
        {
            RxPacketByteCount = 0;
            RxCheckSum = 0;

            Zero(ref UAVXPacket, 256);
            RxPacketTag = UnknownPacketTag;

            RxPacketLength = 2; // set as minimum
            PacketRxState = WaitRxSentinel;

            FrSkyPacketRxState = WaitRxSentinel;
            FrSkyPacketReceived = false;
        }

        void AddToBuffer(byte ch)
        {
            bool RxPacketError;

            UAVXPacket[RxPacketByteCount++] = ch;
            if (RxPacketByteCount == 1)
            {
                RxPacketTag = ch;
                PacketRxState = WaitRxBody;
            }
            else
                if (RxPacketByteCount == 2)
                {
                    RxPacketLength = ch;
                    PacketRxState = WaitRxBody;
                }
                else
                    if (RxPacketByteCount >= (RxPacketLength + 3))
                    {
                        RxPacketError = RxCheckSum != 0;

                        CheckSumError = RxPacketError;

                        if (CheckSumError)
                            RxCheckSumErrors++;

                        if (!RxPacketError)
                        {
                            PacketReceived = true;
                            ReceivedPacketTag = RxPacketTag;
                        }
                        PacketRxState = WaitRxSentinel;
                        //   InitPollPacket(); 
                    }
                    else
                        PacketRxState = WaitRxBody;
        }

        void ParseFrSkyPacket(byte ch)
        {
            switch (FrSkyPacketRxState)
            {
                case WaitRxSentinel:
                    if (ch == 0x7e)
                    {
                        FrSkyRxPacketTag = FrSkyPacketTag;
                        FrSkyPacketRxState = WaitRxTag;
                    }
                    break;
                case WaitRxTag:
                    if (ch != 0x7e)
                    {
                        FrSkyReceivedPacketTag = ch;
                        FrSkyRxPacketByteCount = 0;
                        FrSkyPacketRxState = WaitRxBody;
                    }
                    break;
                case WaitRxBody:
                    if ( ((ch == 0x7d) || (ch == 0x7e)) && ( FrSkyRxPacketByteCount != 8 ))
                        FrSkyPacketRxState = WaitRxESC;
                    else
                    {
                        FrSkyPacket[FrSkyRxPacketByteCount++] = ch;
                        if (FrSkyRxPacketByteCount == 9)
                        {
                            if (ch != 0x7e)
                                RxFrSkyErrors++;
                            else
                                FrSkyPacketReceived = true;
                            FrSkyPacketRxState = WaitRxSentinel;
                        }
                    }
                    break;
                case WaitRxESC:
                    ch ^= 0x20;
                    FrSkyPacket[FrSkyRxPacketByteCount++] = ch;
                    FrSkyPacketRxState = WaitRxBody;
                    break;

                default: FrSkyPacketRxState = WaitRxSentinel; break;
            }
        }

        void ParsePacket(byte ch)
        {
            RxCheckSum ^= ch;
            switch (PacketRxState)
            {
                case WaitRxSentinel:
                    if (ch == SOH)
                    {
                        InitPollPacket();
                        CheckSumError = false;
                        PacketRxState = WaitRxBody;
                    }
                    break;
                case WaitRxBody:
                    if (ch == ESC)
                        PacketRxState = WaitRxESC;
                    else
                        if (ch == SOH) // unexpected start of packet
                        {
                            RxLengthErrors++;

                            InitPollPacket();
                            PacketRxState = WaitRxBody;
                        }
                        else
                            if (ch == EOT) // unexpected end of packet 
                            {
                                RxLengthErrors++;
                                PacketRxState = WaitRxSentinel;
                            }
                            else
                                AddToBuffer(ch);
                    break;
                case WaitRxESC:
                    AddToBuffer(ch);
                    break;
                default: PacketRxState = WaitRxSentinel; break;
            }
        }

        public void DoAirframeLabels()
        {
            if ( UAVXArm )
            switch (AirframeT)
            {
                case 0: // Quadrocopter
                    Output0Label.Text = "F";
                    Output1Label.Text = "R";
                    Output2Label.Text = "B";
                    Output3Label.Text = "L";
                    break;
                case 1: // Tricopter
                    Output0Label.Text = "F";
                    Output1Label.Text = "R";
                    Output2Label.Text = "L";
                    Output3Label.Text = "S";
                    break;
                case 2: // VTcopter
                    Output0Label.Text = "0";
                    Output1Label.Text = "1";
                    Output2Label.Text = "2";
                    Output3Label.Text = "3";
                    break;
                case 3: // Y6copter
                    Output0Label.Text = "TF";
                    Output1Label.Text = "TR";
                    Output2Label.Text = "TL";
                    Output3Label.Text = "BF";
                    Output4Label.Text = "BR";
                    Output5Label.Text = "BL";
                    break;
                case 4: // Helicopter
                    Output0Label.Text = "Th";
                    Output1Label.Text = "R";
                    Output2Label.Text = "P";
                    Output3Label.Text = "Y";
                    break;
                case 5: // Flying Wing
                    Output0Label.Text = "Th";
                    Output1Label.Text = "RE";
                    Output2Label.Text = "LE";
                    Output3Label.Text = "R";
                    break;
                case 6: // Conventional
                    Output0Label.Text = "Th";
                    Output1Label.Text = "A";
                    Output2Label.Text = "E";
                    Output3Label.Text = "R";
                    break;
                case 7: // Hexacopter
                    Output0Label.Text = "F";
                    Output1Label.Text = "FR";
                    Output2Label.Text = "BR";
                    Output3Label.Text = "B";
                    Output4Label.Text = "BL";
                    Output5Label.Text = "FL";
                    break;
                }
            else
                switch (AirframeT)
                {
                    case 0:
                        Output0Label.Text = "F";
                        Output1Label.Text = "B";
                        Output2Label.Text = "R";
                        Output3Label.Text = "L";
                        Output4Label.Visible = false;
                        Output5Label.Visible = false;
                        OutputT4.Visible = false;
                        OutputT5.Visible = false;
                        break;// Quadrocopter
                    case 1:
                        Output0Label.Text = "F";
                        Output1Label.Text = "S";
                        Output2Label.Text = "R";
                        Output3Label.Text = "L";
                        Output4Label.Visible = false;
                        Output5Label.Visible = false;
                        OutputT4.Visible = false;
                        OutputT5.Visible = false;
                        break;// Tricopter
                    case 2:
                        Output0Label.Text = "0";
                        Output1Label.Text = "1";
                        Output2Label.Text = "2";
                        Output3Label.Text = "3";
                        Output4Label.Visible = false;
                        Output5Label.Visible = false;
                        OutputT4.Visible = false;
                        OutputT5.Visible = false;
                        break;// VTcopter
                    case 3: // Y6copter
                        Output0Label.Text = "TF";
                        Output1Label.Text = "TL";
                        Output2Label.Text = "TR";
                        Output3Label.Text = "BF";
                        Output4Label.Text = "BL";
                        Output5Label.Text = "BR";
                        Output6Label.Visible = false;
                        Output7Label.Visible = false;
                        OutputT6.Visible = false;
                        OutputT7.Visible = false;
                        break;
                    case 4: // Helicopter
                        Output0Label.Text = "Th";
                        Output1Label.Text = "R";
                        Output2Label.Text = "P";
                        Output3Label.Text = "Y";
                        Output4Label.Visible = false;
                        Output5Label.Visible = false;
                        OutputT4.Visible = false;
                        OutputT5.Visible = false;
                        break;
                    case 5: // Flying Wing
                        Output0Label.Text = "Th";
                        Output1Label.Text = "RE";
                        Output2Label.Text = "LE";
                        Output3Label.Text = "R";
                        Output4Label.Visible = false;
                        Output5Label.Visible = false;
                        OutputT4.Visible = false;
                        OutputT5.Visible = false;
                        break;
                    case 6: // Conventional
                        Output0Label.Text = "Th";
                        Output1Label.Text = "A";
                        Output2Label.Text = "E";
                        Output3Label.Text = "R";
                        Output4Label.Visible = false;
                        Output5Label.Visible = false;
                        OutputT4.Visible = false;
                        OutputT5.Visible = false;
                        break;
                    case 7: // Hexacopter
                        break;
                }
        }

        void UpdateFlags()
        {
            if ((Flags[0] & 0x01) != 0)
                AltHoldBox.BackColor = System.Drawing.Color.Green;
            else
                AltHoldBox.BackColor = System.Drawing.Color.Red;
            if ((Flags[0] & 0x02) != 0)
                TurnToWPBox.BackColor = System.Drawing.Color.Green;
            else
                TurnToWPBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[0] & 0x04) != 0)
                GyroFailureBox.BackColor = System.Drawing.Color.Green;
            else
                GyroFailureBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[0] & 0x08) != 0)
                LostModelBox.BackColor = System.Drawing.Color.Red;
            else
                LostModelBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[0] & 0x10) != 0)
                NearLevelBox.BackColor = System.Drawing.Color.Green;
            else
                NearLevelBox.BackColor = System.Drawing.Color.LightSteelBlue;
            if ((Flags[0] & 0x20) != 0)
            {
                LowBatteryBox.BackColor = System.Drawing.Color.Red;
                BatteryVolts.BackColor = System.Drawing.Color.Red;
            }
            else
            {
                LowBatteryBox.BackColor = FlagsGroupBox.BackColor;
                BatteryVolts.BackColor = BatteryGroupBox.BackColor;
            }
            if ((Flags[0] & 0x40) != 0)
                GPSValidBox.BackColor = System.Drawing.Color.Green;
            else
                GPSValidBox.BackColor = System.Drawing.Color.Red;

            if ((Flags[0] & 0x80) != 0)
                NavValidBox.BackColor = System.Drawing.Color.Green;
            else
                NavValidBox.BackColor = System.Drawing.Color.Red;

            //if (UAVXArm)
           // GPSROC.Visible = false;
            //else
            //    GPSROC.Visible = true;

            if ((Flags[1] & 0x01) != 0)
                BaroFailBox.BackColor = System.Drawing.Color.Red;
            else
                BaroFailBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[1] & 0x02) != 0)
                AccFailBox.BackColor = System.Drawing.Color.Red;
            else
                AccFailBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[1] & 0x04) != 0)
                CompassFailBox.BackColor = System.Drawing.Color.Red;
            else
                CompassFailBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[1] & 0x08) != 0)
                GPSFailBox.BackColor = System.Drawing.Color.Red;
            else
                GPSFailBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[1] & 0x10) != 0)
                AttitudeHoldBox.BackColor = System.Drawing.Color.Green;
            else
                AttitudeHoldBox.BackColor = System.Drawing.Color.LightSteelBlue;
            if ((Flags[1] & 0x20) != 0)
                ThrottleMovingBox.BackColor = System.Drawing.Color.LightSteelBlue;
            else
                ThrottleMovingBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[1] & 0x40) != 0)
                HoldingAltBox.BackColor = System.Drawing.Color.Green;
            else
                HoldingAltBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[1] & 0x80) != 0)
                NavigateBox.BackColor = System.Drawing.Color.Green;
            else
                NavigateBox.BackColor = FlagsGroupBox.BackColor;

            if ((Flags[2] & 0x01) != 0)
                ReturnHomeBox.BackColor = System.Drawing.Color.Green;
            else
                ReturnHomeBox.BackColor = FlagsGroupBox.BackColor;

            if ((Flags[2] & 0x02) != 0)
                ProximityBox.BackColor = System.Drawing.Color.Green;
            else
                ProximityBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[2] & 0x04) != 0)
                CloseProximityBox.BackColor = System.Drawing.Color.Green;
            else
                CloseProximityBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[2] & 0x10) != 0)
                UseRTHAutoDescendBox.BackColor = System.Drawing.Color.Green;
            else
                UseRTHAutoDescendBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[2] & 0x20) != 0)
                BaroAltValidBox.BackColor = System.Drawing.Color.Green;
            else
                BaroAltValidBox.BackColor = FlagsGroupBox.BackColor;
            if ((Flags[2] & 0x40) != 0)
            {
                RangefinderAltValidBox.BackColor = System.Drawing.Color.Green;
                RangefinderAltitude.BackColor = AltitudeGroupBox.BackColor;
            }
            else
            {
                RangefinderAltValidBox.BackColor = FlagsGroupBox.BackColor;
                RangefinderAltitude.BackColor = System.Drawing.Color.Orange;
            }
            if ((Flags[2] & 0x80) != 0)
                UsingRangefinderBox.BackColor = System.Drawing.Color.Green;
            else
                UsingRangefinderBox.BackColor = FlagsGroupBox.BackColor;

            if ((Flags[3] & 0x08) != 0)
                SimulationTextBox.Text = "SIMULATION";
            else
                SimulationTextBox.Text = " ";

            if ((Flags[3] & 0x02) != 0)
                PHLockEnBox.BackColor = System.Drawing.Color.Green;
            else
                PHLockEnBox.BackColor = FlagsGroupBox.BackColor;

            if ((Flags[3] & 0x04) != 0)
                FocusLockedBox.BackColor = System.Drawing.Color.Green;
            else
                FocusLockedBox.BackColor = System.Drawing.Color.LightSteelBlue;

            if ((Flags[3] & 0x80) != 0)
                SticksFrozenBox.BackColor = System.Drawing.Color.Orange;
            else
                SticksFrozenBox.BackColor = System.Drawing.Color.Green;
        }

        void DoOrientation()
        {
            if (!DoneOrientation)
            {
                if ((Flags[3] & 0x08) != 0)
                    FlightHeadingOffset = 0.0;
                else
                    FlightHeadingOffset = (OrientT * Math.PI) / 24.0;

                OSO = Math.Sin(FlightHeadingOffset);
                OCO = Math.Cos(FlightHeadingOffset);

                DoneOrientation = true;
            }
        }

        void UpdateFlightState()
        {
            switch (StateT)
            {
                case 0: FlightState.Text = "Starting"; break;
                case 1: FlightState.Text = "Landing"; break;
                case 2: FlightState.Text = "Landed"; break;
                case 3: FlightState.Text = "Shutdown"; break;
                case 4: FlightState.Text = "Flying"; break;
                default: FlightState.Text = "Unknown"; break;
            } // switch
        }

        void UpdateNavState()
        {

            switch (NavStateT)
            {
                case 0: NavState.Text = "Holding";
                    NavState.BackColor = System.Drawing.Color.LightSteelBlue;
                    break;
                case 1: NavState.Text = "Returning";
                    NavState.BackColor = System.Drawing.Color.Lime;
                    break;
                case 2: NavState.Text = "@Home";
                    NavState.BackColor = System.Drawing.Color.Green;
                    break;
                case 3: NavState.Text = "Descending";
                    NavState.BackColor = System.Drawing.Color.Orange;
                    break;
                case 4: NavState.Text = "Touchdown!";
                    NavState.BackColor = System.Drawing.Color.RosyBrown;
                    break;
                case 5: NavState.Text = "Navigating";
                    NavState.BackColor = System.Drawing.Color.Silver;
                    break;
                case 6: NavState.Text = "Loitering";
                    NavState.BackColor = System.Drawing.Color.Gold;
                    break;
                default: NavState.Text = "Unknown"; break;
            } // switch
        }

        void UpdateFailState()
        {                 
            switch (FailStateT)
            {
                case 0: FailState.Text = "Monitor Rx";
                    FailState.BackColor = System.Drawing.Color.Green;
                    break;
                case 1: FailState.Text = "Aborting";
                    FailState.BackColor = System.Drawing.Color.Orange;
                    break;
                case 2: FailState.Text = "Terminating";
                    FailState.BackColor = System.Drawing.Color.Orange;
                    break;
                case 3: FailState.Text = "Terminated";
                    FailState.BackColor = System.Drawing.Color.Red;
                    break;
                case 4: FailState.Text = "Terminating";
                    FailState.BackColor = System.Drawing.Color.Orange;
                    break;
                default: FailState.Text = "Unknown"; break;
            } // switch
        }

        void UpdateAttitude()
        {
            if (UAVXArm)
            {
                RollRate.Text = string.Format("{0:n0}", RollRateT * MILLIRADDEG);
                PitchRate.Text = string.Format("{0:n0}", PitchRateT * MILLIRADDEG);
                YawRate.Text = string.Format("{0:n0}", YawRateT * MILLIRADDEG);
                RollAngle.Text = string.Format("{0:n0}", RollAngleT * MILLIRADDEG);
                PitchAngle.Text = string.Format("{0:n0}", PitchAngleT * MILLIRADDEG);
                YawAngle.Text = string.Format("{0:n0}", YawAngleT * MILLIRADDEG);
            }
            else
            {
                RollRate.Text = string.Format("{0:n0}", RollRateT);
                PitchRate.Text = string.Format("{0:n0}", PitchRateT);
                YawRate.Text = string.Format("{0:n0}", YawRateT);
                RollAngle.Text = string.Format("{0:n0}", -RollAngleT / AttitudeToDegrees);
                PitchAngle.Text = string.Format("{0:n0}", -PitchAngleT / AttitudeToDegrees);
                YawAngle.Text = string.Format("{0:n0}", YawAngleT / AttitudeToDegrees);
            }
        }

        void UpdateAccelerations()
        {
            if (UAVXArm)
            {
                LRAcc.Text = string.Format("{0:n2}", (float)LRAccT * 0.001);
                FBAcc.Text = string.Format("{0:n2}", (float)FBAccT * 0.001);
                DUAcc.Text = string.Format("{0:n2}", (float)DUAccT * 0.001);

                FBAccLabel.Text = "BF";
                DUAccLabel.Text = "UD";
            }
            else
            {
                LRAcc.Text = string.Format("{0:n2}", (float)LRAccT / 1024.0);
                FBAcc.Text = string.Format("{0:n2}", (float)FBAccT / 1024.0);
                DUAcc.Text = string.Format("{0:n2}", (float)DUAccT / 1024.0);

                FBAccLabel.Text = "FB";
                DUAccLabel.Text = "DU";
            }
        }

        void UpdateCompensation()
        {
            if (UAVXArm)
            {
                FBCompLabel.Text = "BF";
                DUCompLabel.Text = "UD";
            }
            else
            {
                FBCompLabel.Text = "FB";
                DUCompLabel.Text = "DU";
            }
            LRComp.Text = string.Format("{0:n0}", LRCompT * OUTMaximumScale);
            FBComp.Text = string.Format("{0:n0}", FBCompT * OUTMaximumScale);
            DUComp.Text = string.Format("{0:n0}", DUCompT * OUTMaximumScale);
            AltComp.Text = string.Format("{0:n0}", AltCompT * OUTMaximumScale);
        }

        void UpdateAirframe()
        {
            if (UAVXArm)
                switch (AirframeT)
                {
                    case 0: Airframe.Text = "Arm Quadrocopter"; break;
                    case 1: Airframe.Text = "Arm Tricopter"; break;
                    case 2: Airframe.Text = "Arm VTCopter"; break;
                    case 3: Airframe.Text = "Arm Y6Copter"; break;
                    case 4: Airframe.Text = "Arm Helicopter"; break;
                    case 5: Airframe.Text = "Arm Flying Wing"; break;
                    case 6: Airframe.Text = "Arm Conventional"; break;
                    case 7: Airframe.Text = "Arm Hexacopter"; break;
                }
            else
                switch (AirframeT)
                {
                    case 0: Airframe.Text = "Quadrocopter"; break;
                    case 1: Airframe.Text = "Tricopter"; break;
                    case 2: Airframe.Text = "VTcopter"; break;
                    case 3: Airframe.Text = "Y6copter"; break;
                    case 4: Airframe.Text = "Helicopter"; break;
                    case 5: Airframe.Text = "Flying Wing"; break;
                    case 6: Airframe.Text = "Conventional"; break;
                    case 7: Airframe.Text = "Hexacopter"; break;
                }
          }

        void UpdateBattery()
        {
            if (UAVXArm)
            {
                BatteryVolts.Text = string.Format("{0:n1}", (float)BatteryVoltsT * 0.1);
                BatteryCurrent.Text = string.Format("{0:n1}", (float)BatteryCurrentT * 0.1);
                BatteryCharge.Text = string.Format("{0:n0}", (float)BatteryChargeT);
            }
            else
            {
                BatteryVolts.Text = string.Format("{0:n1}", (float)BatteryVoltsT * 27.73 / 1024.0); // ADC units 5V * (10K+2K2)/2K2.
                BatteryCurrent.Text = string.Format("{0:n1}", ((float)BatteryCurrentT * CurrentSensorMax) / 1024.0); // ADC units sent
                BatteryCharge.Text = string.Format("{0:n0}", (float)BatteryChargeT); // mAH // converted as it is used on board
            }
        }

        void UpdateWhere()
        {
            if (!FirstGPSCoordinates)
            {
                WhereBearing.BackColor = NavGroupBox.BackColor;
                WhereDistance.BackColor = NavGroupBox.BackColor;

                NorthDiff = GPSLatitudeT - OriginLatitude;

                LongitudeCorrection = Math.Cos(Math.PI / 180.0 * (GPSLatitudeT + OriginLatitude) / 12000000.0);

                EastDiff = (GPSLongitudeT - OriginLongitude) * LongitudeCorrection;
                WhereDirection = Math.Atan2(EastDiff, NorthDiff) * MILLIRADDEG;
                while (WhereDirection < 0)
                    WhereDirection += 360.0;
                WhereBearing.Text = string.Format("{0:n0}", WhereDirection);
                Distance = ConvertGPSToM(Math.Sqrt(NorthDiff * NorthDiff + EastDiff * EastDiff));
                WhereDistance.Text = string.Format("{0:n0}", Distance);
                if (Distance > MaximumRangeLimit)
                    WhereDistance.BackColor = System.Drawing.Color.Orange;
                else
                    WhereDistance.BackColor = NavGroupBox.BackColor;
            }
            else
            {
                WhereBearing.BackColor = System.Drawing.Color.Orange;
                WhereDistance.BackColor = System.Drawing.Color.Orange;

                WhereBearing.Text = "?";
                WhereDistance.Text = "?";
            }
        }

        void UpdateControls()
        {
            DesiredThrottle.Text = string.Format("{0:n0}", ((float)DesiredThrottleT * 100.0) / RCMaximum);
            DesiredRoll.Text = string.Format("{0:n0}", ((float)DesiredRollT * 200.0) / RCMaximum);
            DesiredPitch.Text = string.Format("{0:n0}", ((float)DesiredPitchT * 200.0) / RCMaximum);
            DesiredYaw.Text = string.Format("{0:n0}", ((float)DesiredYawT * 200.0) / RCMaximum);
        }

        void DoMotorsAndTime(short p)
        {
            short m;

             if ( UAVXArm )
                {
                    m = p;
                    for (m = 0; m < 8; m++)
                        OutputT[m] = ExtractShort(ref UAVXPacket, (byte)(p+m*2));

                    MissionTimeMilliSecT = ExtractInt24(ref UAVXPacket, (byte)(p+8));
                }
                else
                {
                    m = p;
                    for (m = 0; m < 6 ; m++)
                        OutputT[m] = ExtractByte(ref UAVXPacket, (byte)(p+m));

                    OutputT[6] = OutputT[4];
                    OutputT[7] = OutputT[5];

                    MissionTimeMilliSecT = ExtractInt24(ref UAVXPacket, (byte)(p+6));
                }
        }

        void UpdateMotors()
        {

            OutputT0.Text = string.Format("{0:n0}", OutputT[0] * OUTMaximumScale);
            OutputT1.Text = string.Format("{0:n0}", OutputT[1] * OUTMaximumScale);
            OutputT2.Text = string.Format("{0:n0}", OutputT[2] * OUTMaximumScale);
            OutputT3.Text = string.Format("{0:n0}", OutputT[3] * OUTMaximumScale);

            OutputT4.Text = string.Format("{0:n0}", OutputT[4] * OUTMaximumScale);
            OutputT5.Text = string.Format("{0:n0}", OutputT[5] * OUTMaximumScale);
            OutputT6.Text = string.Format("{0:n0}", OutputT[6] * OUTMaximumScale);
            OutputT7.Text = string.Format("{0:n0}", OutputT[7] * OUTMaximumScale);
        }

        void UpdateAltitude()
        {
            BaroROC.Text = string.Format("{0:n1}", (float)BaroROCT * 0.1);
            RelBaroAltitude.Text = string.Format("{0:n1}", (float)RelBaroAltitudeT * 0.1);

            if (RelBaroAltitudeT > (MaximumAltitudeLimit * 10))
                RelBaroAltitude.BackColor = System.Drawing.Color.Orange;
            else
                RelBaroAltitude.BackColor = AltitudeGroupBox.BackColor;

            if (BaroROCT < -100) // 1M/S
                BaroROC.BackColor = System.Drawing.Color.Orange;
            else
                BaroROC.BackColor = AltitudeGroupBox.BackColor;

            if (UAVXArm)
                RangefinderAltitude.Text = string.Format("{0:n2}", (float)RangefinderAltitudeT * 0.01);
            else
                RangefinderAltitude.Text = string.Format("{0:n2}", (float)RangefinderAltitudeT * 0.1);

            if ((Flags[2] & 0x80) != 0)
            {
                CurrAlt = RangefinderAltitudeT / 10;
                AltitudeSource.Text = "Rangefinder";
            }
            else
                if (((Flags[2] & 0x08) != 0) && ((Flags[0] & 0x80) != 0))
                {
                    CurrAlt = GPSRelAltitudeT;
                    AltitudeSource.Text = "GPS";
                }
                else
                {
                    CurrAlt = RelBaroAltitudeT;
                    AltitudeSource.Text = "Barometer";
                }

            CurrentAltitude.Text = string.Format("{0:n0}", (float)CurrAlt * 0.1);
            if ((CurrAlt * 0.1) > MaximumAltitudeLimit)
                CurrentAltitude.BackColor = System.Drawing.Color.Orange;
            else
                CurrentAltitude.BackColor = NavGroupBox.BackColor;

            AltError = CurrAlt - DesiredAltitudeT;
            AltitudeError.Text = string.Format("{0:n1}", (float)AltError * 0.1);
        }

        void ProcessFrSkyPacket()
        {
            short b, nb;
            double RxVolts;
            short LQ;

            FrSkyPacketReceived = false;
            switch ( FrSkyReceivedPacketTag ) {
                case 0xf0:
                case 0xf1:
                case 0xf2:
                case 0xf3:
                case 0xf4:
                case 0xf5:
                case 0xf6:
                case 0xf7:
                    break;
                case 0xf8:    // Threshold request
                    break;
                case 0xf9:    // A2 Threshold

                    break;
                case 0xfa:    // A2 Threshold

                    break;
                case 0xfb:    // A1 Threshold

                    break;
                case 0xfc:    // A1 Threshold
                  
                    break;
                case 0xfd:    // Data packet
                    nb = FrSkyPacket[0];
                    for (b = 2; b < (nb + 2); b++)
                    {
                        ParsePacket(FrSkyPacket[b]);
                        if (PacketReceived)
                            ProcessPacket();
                    }
                    break;
                case 0xfe:    // A1, A2, link quality
                    RxVolts = (FrSkyPacket[0] * 4.0 * 3.3) / 255.0;
                    if ( RxVolts < 4.0 )
                        FrSkyA1.BackColor = System.Drawing.Color.Orange;
                    else
                        if ( RxVolts < 3.5 )
                            FrSkyA1.BackColor = System.Drawing.Color.Red;
                        else
                            FrSkyA1.BackColor = FrSkyBox.BackColor;

                    FrSkyA1.Text = string.Format("{0:n1}", RxVolts );
                    FrSkyA2.Text = string.Format("{0:n1}", ((float)FrSkyPacket[1] * 10.0 * 3.3) / 255.0);

                    LQ = FrSkyPacket[2];
                    if ( LQ < 80 )
                        FrSkyLQ.BackColor = System.Drawing.Color.Orange;
                    else
                        if ( LQ < 70 )
                            FrSkyLQ.BackColor = System.Drawing.Color.Red;
                        else
                            FrSkyLQ.BackColor = FrSkyBox.BackColor;

                    FrSkyLQ.Text = string.Format("{0:n0}", LQ );

                    break;
                case 0xff:

                    break;
                default:
                    break;
            }
        }

        void ProcessPacket()
        {
            byte b;
            short i;

            PacketReceived = false;

            if (DoingLogfileReplay)
                ReplayProgressBar.Value = (int)ReplayProgress;

                switch (RxPacketTag)
                {
                case UAVXMinPacketTag:

                    for (i = 2; i < (NoOfFlagBytes + 2); i++)
                        Flags[i - 2] = ExtractByte(ref UAVXPacket, i);

                    StateT = ExtractByte(ref UAVXPacket, 8);               
                    NavStateT = ExtractByte(ref UAVXPacket, 9);
                    FailStateT = ExtractByte(ref UAVXPacket, 10);
                    BatteryVoltsT = ExtractShort(ref UAVXPacket, 11);
                    BatteryCurrentT = ExtractShort(ref UAVXPacket, 13);
                    BatteryChargeT = ExtractShort(ref UAVXPacket, 15);
                    RollAngleT = ExtractShort(ref UAVXPacket, 17);
                    PitchAngleT = ExtractShort(ref UAVXPacket, 19);
                    RelBaroAltitudeT = ExtractInt24(ref UAVXPacket, 21);
                    RangefinderAltitudeT = ExtractShort(ref UAVXPacket, 24);  
                    HeadingT = ExtractShort(ref UAVXPacket, 26);
                    GPSLatitudeT = ExtractInt(ref UAVXPacket, 28);
                    GPSLongitudeT = ExtractInt(ref UAVXPacket, 32);
                    AirframeT = ExtractByte(ref UAVXPacket, 36);
                    OrientT = ExtractByte(ref UAVXPacket, 37);
                    MissionTimeMilliSecT = ExtractInt24(ref UAVXPacket, 38);

                    DesiredAltitudeT = 0;
                    BaroROCT = 0;

                    UAVXArm = (AirframeT & 0x80) != 0;
                    AirframeT &= 0x7f;

                    UpdateFlightState();
                    UpdateNavState();
                    UpdateFailState();
                    UpdateBattery();
                    UpdateWhere();
                    UpdateFlags();
                    UpdateAirframe();

                    UpdateAltitude();
                    UpdateAttitude();

                    DoOrientation();

                    Heading.Text = string.Format("{0:n0}", (float)HeadingT * MILLIRADDEG);

                    GPSLongitude.Text = string.Format("{0:n6}", (double)GPSLongitudeT / 6000000.0);
                    GPSLatitude.Text = string.Format("{0:n6}", (double)GPSLatitudeT / 6000000.0);

                    if ((Flags[0] & 0x40) != 0) // GPSValid
                    {
                        GPSLongitude.BackColor = NavGroupBox.BackColor;
                        GPSLatitude.BackColor = NavGroupBox.BackColor;
                    }
                    else
                    {
                        GPSLongitude.BackColor = System.Drawing.Color.Orange;
                        GPSLatitude.BackColor = System.Drawing.Color.Orange;
                    }

                    if (FirstGPSCoordinates && ((Flags[0] & 0x80) != 0))
                    {
                        FirstGPSCoordinates = false;
                        OriginLatitude = GPSLatitudeT;
                        OriginLongitude = GPSLongitudeT;
                    }

                    MissionTimeSec.Text = string.Format("{0:n3}", MissionTimeMilliSecT * 0.001);

                    WriteTextLogFile(); // no other packets

                    break;
                case UAVXParamsPacketTag:
                    ParamSet = ExtractByte(ref UAVXPacket, 2);
                    ParamNo = ExtractByte(ref UAVXPacket, 3);
                    if (ParamSet == 0)
                    {
                        SaveTextParamFileStreamWriter.Write("Params," + ParamSet + ",");

                        for (b = 0; b < ParamNo; b++)
                            SaveTextParamFileStreamWriter.Write(Params[ParamSet - 1, b] + ",");
                        SaveTextParamFileStreamWriter.WriteLine();
                    }
                    else
                        if (UAVXArm)
                            Params[ParamSet - 1, ParamNo] = ExtractInt(ref UAVXPacket, 4) * 0.0001;
                        else
                            Params[ParamSet - 1, ParamNo] = ExtractByte(ref UAVXPacket, 4);

                    break;
                case UAVXStatsPacketTag:
                    StatsPacketsReceived++;
                    I2CFailsT = ExtractShort(ref UAVXPacket, 2);
                    GPSFailsT = ExtractShort(ref UAVXPacket, 4);
                    AccFailsT = ExtractShort(ref UAVXPacket, 6);
                    GyroFailsT = ExtractShort(ref UAVXPacket, 8);
                    CompassFailsT = ExtractShort(ref UAVXPacket, 10);
                    BaroFailsT = ExtractShort(ref UAVXPacket, 12);
                    I2CESCFailsT = ExtractShort(ref UAVXPacket, 14);

                    RCFailSafesT = ExtractShort(ref UAVXPacket, 16);

                    GPSAltitudeT = ExtractShort(ref UAVXPacket, 18);
                    GPSMaxVelT = ExtractShort(ref UAVXPacket, 20);
                    GPSMinSatsT = ExtractShort(ref UAVXPacket, 22);
                    GPSMaxSatsT = ExtractShort(ref UAVXPacket, 24);
                    GPSMinHDiluteT = ExtractShort(ref UAVXPacket, 26);
                    GPSMaxHDiluteT = ExtractShort(ref UAVXPacket, 28);

                    BaroRelAltitudeT = ExtractShort(ref UAVXPacket, 30);
                    BaroMinROCT = ExtractShort(ref UAVXPacket, 32);
                    BaroMaxROCT = ExtractShort(ref UAVXPacket, 34);

                    MinTempT = ExtractShort(ref UAVXPacket, 36);
                    MaxTempT = ExtractShort(ref UAVXPacket, 38);

                    BadT = ExtractShort(ref UAVXPacket, 40);
                    AirframeT = ExtractByte(ref UAVXPacket, 42);

                    OrientT = ExtractByte(ref UAVXPacket, 43);
                    BadNumT = ExtractShort(ref UAVXPacket, 44);

                    UAVXArm = (AirframeT & 0x80) != 0;
                    AirframeT &= 0x7f;

                    I2CFailS.Text = string.Format("{0:n0}", I2CFailsT);
                    GPSFailS.Text = string.Format("{0:n0}", GPSFailsT);
                    AccFailS.Text = string.Format("{0:n0}", AccFailsT);
                    GyroFailS.Text = string.Format("{0:n0}", GyroFailsT);
                    CompassFailS.Text = string.Format("{0:n0}", CompassFailsT);
                    BaroFailS.Text = string.Format("{0:n0}", BaroFailsT);
                    I2CESCFailS.Text = string.Format("{0:n0}", I2CESCFailsT);
                    RCFailSafeS.Text = string.Format("{0:n0}", RCFailSafesT);

                    GPSAltitudeS.Text = string.Format("{0:n1}", (float)GPSAltitudeT * 0.1);
                    GPSMaxVelS.Text = string.Format("{0:n1}", (float)GPSMaxVelT * 0.1);
                    GPSMinSatS.Text = string.Format("{0:n0}", GPSMinSatsT);
                    GPSMaxSatS.Text = string.Format("{0:n0}", GPSMaxSatsT);
                    GPSMinHDiluteS.Text = string.Format("{0:n2}", (float)GPSMinHDiluteT * 0.01);
                    GPSMaxHDiluteS.Text = string.Format("{0:n2}", (float)GPSMaxHDiluteT * 0.01);

                    BaroRelAltitudeS.Text = string.Format("{0:n1}", (float)BaroRelAltitudeT * 0.1);
                    BaroMinROCS.Text = string.Format("{0:n1}", (float)BaroMinROCT * 0.1);
                    BaroMaxROCS.Text = string.Format("{0:n1}", (float)BaroMaxROCT * 0.1);
                    MinTempS.Text = string.Format("{0:n1}", (float)MinTempT * 0.1);
                    MaxTempS.Text = string.Format("{0:n1}", (float)MaxTempT * 0.1);

                    BadS.Text = string.Format("{0:n0}", BadT);

                    UpdateAirframe();

                    DoOrientation();

                    break;
                case UAVXFlightPacketTag:
                    FlightPacketsReceived++;

                    for (i = 2; i < (NoOfFlagBytes + 2); i++)
                        Flags[i - 2] = ExtractByte(ref UAVXPacket, i);

                    StateT = ExtractByte(ref UAVXPacket, 8);
                    BatteryVoltsT = ExtractShort(ref UAVXPacket, 9);
                    BatteryCurrentT = ExtractShort(ref UAVXPacket, 11);
                    BatteryChargeT = ExtractShort(ref UAVXPacket, 13);
                    RCGlitchesT = ExtractShort(ref UAVXPacket, 15);
                    DesiredThrottleT = ExtractShort(ref UAVXPacket, 17);
                    DesiredRollT = ExtractShort(ref UAVXPacket, 19);
                    DesiredPitchT = ExtractShort(ref UAVXPacket, 21);
                    DesiredYawT = ExtractShort(ref UAVXPacket, 23);
                    RollRateT = ExtractShort(ref UAVXPacket, 25);
                    PitchRateT = ExtractShort(ref UAVXPacket, 27);
                    YawRateT = ExtractShort(ref UAVXPacket, 29);
                    RollAngleT = ExtractShort(ref UAVXPacket, 31);
                    PitchAngleT = ExtractShort(ref UAVXPacket, 33);
                    YawAngleT = ExtractShort(ref UAVXPacket, 35);
                    LRAccT = ExtractShort(ref UAVXPacket, 37);
                    FBAccT = ExtractShort(ref UAVXPacket, 39);
                    DUAccT = ExtractShort(ref UAVXPacket, 41);
                    LRCompT = ExtractSignedByte(ref UAVXPacket, 43);
                    FBCompT = ExtractSignedByte(ref UAVXPacket, 44);
                    DUCompT = ExtractSignedByte(ref UAVXPacket, 45);
                    AltCompT = ExtractSignedByte(ref UAVXPacket, 46);

                    DoMotorsAndTime(47);

                    UpdateFlags();
                    UpdateFlightState();

                    UpdateControls();
                    UpdateAttitude();
                    UpdateAccelerations();
                    UpdateCompensation();
                    UpdateMotors();

                    DoAirframeLabels();

                    RCGlitches.Text = string.Format("{0:n0}", RCGlitchesT);
                    if (RCGlitchesT > 20)
                        RCGlitches.BackColor = System.Drawing.Color.Orange;
                    else
                        RCGlitches.BackColor = EnvGroupBox.BackColor;

                    MissionTimeSec.Text = string.Format("{0:n3}", MissionTimeMilliSecT * 0.001);

                    UpdateBattery();

                    WriteTextLogFile();
                    break;
                case UAVXControlPacketTag:
                    ControlPacketsReceived++;

                    DesiredThrottleT = ExtractShort(ref UAVXPacket, 2);
                    DesiredRollT = ExtractShort(ref UAVXPacket, 4);
                    DesiredPitchT = ExtractShort(ref UAVXPacket, 6);
                    DesiredYawT = ExtractShort(ref UAVXPacket, 8);
                    RollRateT = ExtractShort(ref UAVXPacket, 10);
                    PitchRateT = ExtractShort(ref UAVXPacket, 12);
                    YawRateT = ExtractShort(ref UAVXPacket, 14);
                    RollAngleT = ExtractShort(ref UAVXPacket, 16);
                    PitchAngleT = ExtractShort(ref UAVXPacket, 18);
                    YawAngleT = ExtractShort(ref UAVXPacket, 20);
                    LRAccT = ExtractShort(ref UAVXPacket, 22);
                    FBAccT = ExtractShort(ref UAVXPacket, 24);
                    DUAccT = ExtractShort(ref UAVXPacket, 26);

                    DoMotorsAndTime(28);

                    UpdateControls();
                    UpdateAttitude();
                    UpdateAccelerations();
                    UpdateCompensation();
                    UpdateMotors();

                    DoAirframeLabels();

                    MissionTimeSec.Text = string.Format("{0:n3}", MissionTimeMilliSecT * 0.001);

                    WriteTextLogFile();
                    break;
                case UAVXNavPacketTag:
                    NavPacketsReceived++;
                    NavStateT = ExtractByte(ref UAVXPacket, 2);
                    FailStateT = ExtractByte(ref UAVXPacket, 3);


                    GPSNoOfSatsT = ExtractByte(ref UAVXPacket, 4);
                    GPSFixT = ExtractByte(ref UAVXPacket, 5);
                    CurrWPT = ExtractByte(ref UAVXPacket, 6);
                    BaroROCT = ExtractShort(ref UAVXPacket, 7);
                    RelBaroAltitudeT = ExtractInt24(ref UAVXPacket, 9);
                    GPSHeadingT = ExtractShort(ref UAVXPacket, 12);
                    RangefinderAltitudeT = ExtractShort(ref UAVXPacket, 14);
                    GPSHDiluteT = ExtractShort(ref UAVXPacket, 16);
                    HeadingT = ExtractShort(ref UAVXPacket, 18);
                    DesiredCourseT = ExtractShort(ref UAVXPacket, 20);
                    GPSVelT = ExtractShort(ref UAVXPacket, 22);
                    GPSROCT = ExtractShort(ref UAVXPacket, 24);
                    GPSRelAltitudeT = ExtractInt24(ref UAVXPacket, 26);
                    GPSLatitudeT = ExtractInt(ref UAVXPacket, 29);
                    GPSLongitudeT = ExtractInt(ref UAVXPacket, 33);
                    DesiredAltitudeT = ExtractInt24(ref UAVXPacket, 37);
                    DesiredLatitudeT = ExtractInt(ref UAVXPacket, 40);
                    DesiredLongitudeT = ExtractInt(ref UAVXPacket, 44);
                    NavStateTimeoutT = ExtractInt24(ref UAVXPacket, 48);

                    AmbientTempT = ExtractShort(ref UAVXPacket, 51);
                    GPSMissionTimeT = ExtractInt(ref UAVXPacket, 53);

                    NavSensitivityT = ExtractByte(ref UAVXPacket, 57);
                    NavRCorrT = ExtractSignedByte(ref UAVXPacket, 58);
                    NavPCorrT = ExtractSignedByte(ref UAVXPacket, 59);
                    NavYCorrT = ExtractSignedByte(ref UAVXPacket, 60);

                    UpdateNavState();
                    UpdateFailState();

                    if (FirstGPSCoordinates && ((Flags[0] & 0x80) != 0))
                    {
                        FirstGPSCoordinates = false;
                        OriginLatitude = GPSLatitudeT;
                        OriginLongitude = GPSLongitudeT;
                    }

                    GPSNoOfSats.Text = string.Format("{0:n0}", GPSNoOfSatsT);
                    if (GPSNoOfSatsT < 6)
                        GPSNoOfSats.BackColor = System.Drawing.Color.Orange;
                    else
                        GPSNoOfSats.BackColor = GPSStatBox.BackColor;

                    GPSFix.Text = string.Format("{0:n0}", GPSFixT);
                    if (GPSFixT < 2)
                        GPSFix.BackColor = System.Drawing.Color.Orange;
                    else
                        GPSFix.BackColor = GPSStatBox.BackColor;

                    GPSHDilute.Text = string.Format("{0:n2}", (float)GPSHDiluteT * 0.01);
                    if (GPSHDiluteT > 130)
                        GPSHDilute.BackColor = System.Drawing.Color.Orange;
                    else
                        GPSHDilute.BackColor = GPSStatBox.BackColor;

                    CurrWP.Text = string.Format("{0:n0}", CurrWPT);
                    //pad1.Text = string.Format("{0:n0}", ExtractByte(ref UAVXPacket, 7));

                    Heading.Text = string.Format("{0:n0}", (float)HeadingT * MILLIRADDEG);

                    UpdateAltitude();

                    GPSVel.Text = string.Format("{0:n1}", (double)GPSVelT * 0.1); // dM/Sec
                    //GPSROC.Text = string.Format("{0:n1}", (float)GPSROCT * 0.1);
                    GPSHeading.Text = string.Format("{0:n0}", (float)GPSHeadingT * MILLIRADDEG);
                    GPSRelAltitude.Text = string.Format("{0:n1}", (double)GPSRelAltitudeT * 0.1);
                    GPSLongitude.Text = string.Format("{0:n6}", (double)GPSLongitudeT / 6000000.0);
                    GPSLatitude.Text = string.Format("{0:n6}", (double)GPSLatitudeT / 6000000.0);

                    if ((Flags[0] & 0x40) != 0) // GPSValid
                    {
                        GPSVel.BackColor = NavGroupBox.BackColor;
                       // GPSROC.BackColor = NavGroupBox.BackColor;
                        GPSHeading.BackColor = NavGroupBox.BackColor;
                        GPSRelAltitude.BackColor = NavGroupBox.BackColor;
                        GPSLongitude.BackColor = NavGroupBox.BackColor;
                        GPSLatitude.BackColor = NavGroupBox.BackColor;
                        WayHeading.BackColor = NavGroupBox.BackColor;
                        DistanceToDesired.BackColor = NavGroupBox.BackColor;

                        WayHeading.Text = string.Format("{0:n0}", (float)DesiredCourseT * MILLIRADDEG);

                        LongitudeCorrection = Math.Cos(Math.PI / 180.0 * (DesiredLatitudeT + GPSLatitudeT) / 12000000.0);

                        NorthDiff = (double)(DesiredLatitudeT - GPSLatitudeT); // scale up to decimetres after conversion
                        EastDiff = (double)(DesiredLongitudeT - GPSLongitudeT) * LongitudeCorrection;

                        Distance = ConvertGPSToM(Math.Sqrt(NorthDiff * NorthDiff + EastDiff * EastDiff));
                        DistanceToDesired.Text = string.Format("{0:n1}", Distance);
                    }
                    else
                    {
                        GPSVel.BackColor = System.Drawing.Color.Orange;
                       // GPSROC.BackColor = System.Drawing.Color.Orange;
                        GPSHeading.BackColor = System.Drawing.Color.Orange;
                        GPSRelAltitude.BackColor = System.Drawing.Color.Orange;
                        GPSLongitude.BackColor = System.Drawing.Color.Orange;
                        GPSLatitude.BackColor = System.Drawing.Color.Orange;
                        WayHeading.BackColor = System.Drawing.Color.Orange;
                        DistanceToDesired.BackColor = System.Drawing.Color.Orange;

                        WayHeading.Text = "?";
                        DistanceToDesired.Text = "?";
                    }

                    UpdateWhere();
              
                    if (NavStateTimeoutT >= 0)
                        NavStateTimeout.Text = string.Format("{0:n0}", (float)NavStateTimeoutT * 0.001);
                    else
                        NavStateTimeout.Text = " ";

                    AmbientTemp.Text = string.Format("{0:n1}", AmbientTempT * 0.1);

                    NavSensitivity.Text = string.Format("{0:n0}", NavSensitivityT * OUTMaximumScale);
                    NavRCorr.Text = string.Format("{0:n0}", NavRCorrT * OUTMaximumScale);
                    NavPCorr.Text = string.Format("{0:n0}", NavPCorrT * OUTMaximumScale);
                    NavYCorr.Text = string.Format("{0:n0}", NavYCorrT * OUTMaximumScale);

                    if (AmbientTempT < 0.0)
                        AmbientTemp.BackColor = System.Drawing.Color.LightSteelBlue;
                    else
                        if (AmbientTempT > 300.0)
                            AmbientTemp.BackColor = System.Drawing.Color.Orange;
                        else
                            AmbientTemp.BackColor = EnvGroupBox.BackColor;

                    break;
                default: break;
            } // switch


            if ( ( RxPacketTag == UAVXControlPacketTag ) || ( RxPacketTag == UAVXFlightPacketTag ) || ( RxPacketTag == UAVXMinPacketTag ) )
            {
                FlightRoll = PitchAngleT * OSO + RollAngleT * OCO;
                FlightPitch = PitchAngleT * OCO - RollAngleT * OSO;

                FlightRoll = FlightRollp + (FlightRoll - FlightRollp) * 0.5;
                FlightPitch = FlightPitchp + (FlightPitch - FlightPitchp) * 0.5; // 0.5 1Hz 0.8 5Hz @ 0.2

                FlightRollp = FlightRoll;
                FlightPitchp = FlightPitch;

                if (UAVXArm)
                    attitudeIndicatorInstrumentControl1.SetAttitudeIndicatorParameters(
                        FlightPitch * MILLIRADDEG, -FlightRoll * MILLIRADDEG);
                else
                    attitudeIndicatorInstrumentControl1.SetAttitudeIndicatorParameters(
                        FlightPitch / AttitudeToDegrees, -FlightRoll / AttitudeToDegrees);
            }

            if (RxPacketTag == UAVXNavPacketTag )
            {
                FlightHeading = (int)((HeadingT + FlightHeadingOffset) * MILLIRADDEG);
                if (FlightHeading >= 360) FlightHeading -= 360;

                headingIndicatorInstrumentControl1.SetHeadingIndicatorParameters(FlightHeading);
            }
        }

        public void UAVXReadTelemetry(object sender, EventArgs e)
        {
            byte b;

            while (RxHead != RxTail)
            {
                b = RxQueue[RxHead];
                RxHead++;
                RxHead &= RxQueueMask;

                if (FrSkycheckBox1.Checked)
                {
                    ParseFrSkyPacket(b);
                    if (FrSkyPacketReceived)
                        ProcessFrSkyPacket();
                }
                else
                {
                    ParsePacket(b);
                    if (PacketReceived)
                        ProcessPacket();
                }
                RxTypeErr.Text = string.Format("{0:n0}", RxIllegalErrors);
                RxCSumErr.Text = string.Format("{0:n0}", RxCheckSumErrors);
                RxLenErr.Text = string.Format("{0:n0}", RxLengthErrors);
                RxFrSkyErr.Text = string.Format("{0:n0}", RxFrSkyErrors);
            }
    
            ReadingTelemetry = false;
        }

        void WriteTextLogFile()
        {
            short i;

            SaveTextLogFileStreamWriter.Write("Flight," +

            (Flags[0] & 0x01) + "," + //NavAltitudeHold	
            ((Flags[0] & 0x02) >> 1) + "," + //TurnToWP			
            ((Flags[0] & 0x04) >> 2) + "," + //GyroFailure
            ((Flags[0] & 0x08) >> 3) + "," + //LostModel
            ((Flags[0] & 0x10) >> 4) + "," + //NearLevel
            ((Flags[0] & 0x20) >> 5) + "," + //LowBatt
            ((Flags[0] & 0x40) >> 6) + "," + //GPSValid
            ((Flags[0] & 0x80) >> 7) + "," + //NavValid

            (Flags[1] & 0x01) + "," + //BaroFailure
            ((Flags[1] & 0x02) >> 1) + "," + //AccFailure
            ((Flags[1] & 0x04) >> 2) + "," + //CompassFailure
            ((Flags[1] & 0x08) >> 3) + "," + //GPSFailure
            ((Flags[1] & 0x10) >> 4) + "," + //AttitudeHold
            ((Flags[1] & 0x20) >> 5) + "," + //ThrottleMoving
            ((Flags[1] & 0x40) >> 6) + "," + // Hovering
            ((Flags[1] & 0x80) >> 7) + "," + //Navigate

            (Flags[2] & 0x01) + "," + //ReturnHome
            ((Flags[2] & 0x02) >> 1) + "," + //Proximity
            ((Flags[2] & 0x04) >> 2) + "," + //CloseProximity
            ((Flags[2] & 0x08) >> 3) + "," + //UsingGPSAlt
            ((Flags[2] & 0x10) >> 4) + "," + //UsingRTHAutoDescend
            ((Flags[2] & 0x20) >> 5) + "," + //BaroAltitudeValid
            ((Flags[2] & 0x40) >> 6) + "," + //RangefinderAltitudeValid
            ((Flags[2] & 0x80) >> 7) + "," + //UsingRangefinderAlt

            (Flags[3] & 0x01) + "," + // AllowNavAltitudeHold
            ((Flags[3] & 0x02) >> 1) + "," + // UsingPositionHoldLock
            ((Flags[3] & 0x04) >> 2) + "," + // Ch5Active
            ((Flags[3] & 0x08) >> 3) + "," + // Simulation
            ((Flags[3] & 0x10) >> 4) + "," + // AcquireNewPosition
            ((Flags[3] & 0x20) >> 5) + "," + // MotorsArmed
            ((Flags[3] & 0x40) >> 6) + "," + // NavigationActive
            ((Flags[3] & 0x80) >> 7) + "," + // Sticks Frozen

             (Flags[2] & 0x01) + "," + // Signal
            ((Flags[4] & 0x02) >> 1) + "," + // RCFrameOK
            ((Flags[4] & 0x04) >> 2) + "," + // ParametersValid
            ((Flags[4] & 0x08) >> 3) + "," + // RCNewValues
            ((Flags[4] & 0x10) >> 4) + "," + // NewCommands
            ((Flags[4] & 0x20) >> 5) + "," + // AccelerationsValid
            ((Flags[4] & 0x40) >> 6) + "," + // CompassValid
            ((Flags[4] & 0x80) >> 7) + "," + // CompassMissRead

            (Flags[5] & 0x01) + "," + // UsingPolarCoordinates
            ((Flags[5] & 0x02) >> 1) + "," + // ReceivingGPS
            ((Flags[5] & 0x04) >> 2) + "," + // PacketReceived
            ((Flags[5] & 0x08) >> 3) + "," + // NavComputed
            ((Flags[5] & 0x10) >> 4) + "," + // AltitudeValid 
            ((Flags[5] & 0x20) >> 5) + "," + // UsingSerialPPM 
            ((Flags[5] & 0x40) >> 6) + "," + // UsingTxMode2
            ((Flags[5] & 0x80) >> 7) + ","); // Unused0

            SaveTextLogFileStreamWriter.Write(StateT + "," +
            BatteryVoltsT + "," +
            BatteryCurrentT + "," +
            BatteryChargeT + "," +
            RCGlitchesT + "," +
            DesiredThrottleT + "," +
            DesiredRollT + "," +
            DesiredPitchT + "," +
            DesiredYawT + "," +
            RollRateT + "," +
            PitchRateT + "," +
            YawRateT + "," +
            RollAngleT + "," +
            PitchAngleT + "," +
            YawAngleT + "," +
            LRAccT + "," +
            FBAccT + "," +
            DUAccT + "," +
            LRCompT + "," +
            FBCompT + "," +
            DUCompT + "," +
            AltCompT + ",");

            for (i = 0; i < NoOfOutputs; i++)
                SaveTextLogFileStreamWriter.Write(OutputT[i] + ",");

            SaveTextLogFileStreamWriter.Write(MissionTimeMilliSecT + ",");

            SaveTextLogFileStreamWriter.Write("Nav," +
            NavStateT + "," +
            FailStateT + "," +
            GPSNoOfSatsT + "," +
            GPSFixT + "," +
            CurrWPT + "," +
            BaroROCT + "," +
            RelBaroAltitudeT + "," +
            GPSHeadingT + "," +
            RangefinderAltitudeT + "," +
            GPSHDiluteT + "," +
            HeadingT + "," +
            DesiredCourseT + "," +
            GPSVelT + "," +
            GPSROCT + "," +
            GPSRelAltitudeT + "," +
            GPSLatitudeT + "," +
            GPSLongitudeT + "," +
            DesiredAltitudeT + "," +
            DesiredLatitudeT + "," +
            DesiredLongitudeT + "," +
            NavStateTimeoutT + "," +
            AmbientTempT + "," +
            GPSMissionTimeT + "," +

            NavSensitivityT + "," +
            NavRCorrT + "," +
            NavPCorrT + "," +
            NavYCorrT + ",");

            SaveTextLogFileStreamWriter.WriteLine("Stats," +
            I2CFailsT + "," +
            GPSFailsT + "," +
            AccFailsT + "," +
            GyroFailsT + "," +
            CompassFailsT + "," +
            BaroFailsT + "," +
            I2CESCFailsT + "," +
            RCFailSafesT + "," +

            GPSAltitudeT + "," +
            GPSMaxVelT + "," +
            GPSMinSatsT + "," +
            GPSMaxSatsT + "," +
            GPSMinHDiluteT + "," +
            GPSMaxHDiluteT + "," +

            BaroRelAltitudeT + "," +
            BaroMinROCT + "," +
            BaroMaxROCT + "," +

            MinTempT + "," +
            MaxTempT + "," +

            BadT + "," +
            AirframeT + "," +
            OrientT + "," +
            BadNumT);

        }

        void UAVXCloseTelemetry()
        {
            if (serialPort1.IsOpen)
            {
                serialPort1.Close();

                SaveLogFileBinaryWriter.Flush();
                SaveLogFileBinaryWriter.Close();
                SaveLogFileStream.Close();
                SaveTextLogFileStreamWriter.Flush();
                SaveTextLogFileStreamWriter.Close();
                SaveTextLogFileStream.Close();
                SaveTextParamFileStreamWriter.Flush();
                SaveTextParamFileStreamWriter.Close();
                SaveTextParamFileStream.Close();
            }
        }

        //----------------------------------------------------------------------- 

        public void Zero(ref byte[] a, short len)
        {
            for (int s = 0; s < len; s++)
                a[s] = 0;
        }

        public byte ExtractByte(ref byte[] a, short p)
        {
            return a[p];
        }

        public short ExtractSignedByte(ref byte[] a, short p)
        {
            short temp;

            temp = a[p];
            if (temp > 127)
                temp -= 256;

            return temp;
        }

        public short ExtractShort(ref byte[] a, short p)
        {
            short temp;
            temp = (short)(a[p + 1] << 8);
            temp |= (short)a[p];

            return temp;
        }

        public int ExtractInt24(ref byte[] a, short p)
        {
            int temp;

            temp = ((int)a[p + 2] << 24);
            temp |= ((int)a[p + 1] << 16);
            temp |= (int)a[p] << 8;
            temp /= 256;
            return temp;
        }

        public int ExtractInt(ref byte[] a, short p)
        {
            int temp;

            temp = (int)(a[p + 3] << 24);
            temp |= ((int)a[p + 2] << 16);
            temp |= ((int)a[p + 1] << 8);
            temp |= (int)a[p];
            return temp;
        }

        public double ConvertGPSToM(double c)
        {	// converts to Metres
            // approximately 1.8553257183 cm per LSB at the Equator
            // conversion max is 21Km
            return (c * 0.018553257183);
        }

        private void ReplayNumericUpDown_Changed(object sender, EventArgs e)
        {
            ReplayDelay = 20 - Convert.ToInt16(ReplayNumericUpDown.Text);
        }

    
    }
}