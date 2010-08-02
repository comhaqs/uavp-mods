// UAVPSet
// Copyright (C) 2007  Thorsten Raab
// Email: thorsten.raab@gmx.at
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.IO;
using System.Reflection;

namespace UAVP.UAVPSet
{
    /// <summary>
    /// zentrale klasse um alle Prameter zu speichern
    /// </summary>
    public class ParameterSets
    {
        /// <summary>
        /// initialisierung der Parameter
        /// </summary>
        public ParameterSets()
        {
            // alle Parameter werden vorbereitet
            //TODO: Erweitern wenn neue Parameter hinzukommen
            for (int i = 0; i < parameterForm1.Length; i++) 
            {
                parameterForm1[i].Chapter = "";
                parameterForm2[i].Chapter = "";
                parameterPic1[i].Chapter = "";
                parameterPic2[i].Chapter = "";
                // mindestwerte vorbelegen
                switch (i)
                {
                    case 3:
                    case 4:
                    case 8:
                    case 9:
                    case 13:
                    case 14:
                    case 16:
                        parameterForm1[i].Value = "1";
                        parameterForm2[i].Value = "1";
                        parameterPic1[i].Value = "1";
                        parameterPic2[i].Value = "1";
                        break;
                    case 22:
                        parameterForm1[i].Value = "5";
                        parameterForm2[i].Value = "5";
                        parameterPic1[i].Value = "5";
                        parameterPic2[i].Value = "5";
                        break;
                    default:
                        parameterForm1[i].Value = "0";
                        parameterForm2[i].Value = "0";
                        parameterPic1[i].Value = "0";
                        parameterPic2[i].Value = "0";
                        break;

                }
                parameterForm1[i].Command = "Register " + (i+1).ToString();
                parameterForm2[i].Command = "Register " + (i + 1).ToString();
                parameterPic1[i].Command = "Register " + (i + 1).ToString();
                parameterPic2[i].Command = "Register " + (i + 1).ToString();
                parameterForm1[i].Comment = kurzParameter[i];
                parameterForm2[i].Comment = kurzParameter[i];
                parameterPic1[i].Comment = kurzParameter[i];
                parameterPic2[i].Comment = kurzParameter[i];
            }

        }
            
        // farben um die Felder bei update zu markieren
        public enum Farbe {black, green, orange, red};

        /// <summary>
        /// allgemeine Struktur um die Parameter aufzunehmen
        /// </summary>
        public struct ParameterSetsStruc
        {
            public string Command;
            public string Value;
            public string Comment;
            public string Chapter;
        }

        //TODO: Array erweitern wenn neue Parameter hinzukommen
        // Parameter Sets erzeugen f�r Form
        public ParameterSetsStruc[] parameterForm1 = new ParameterSetsStruc[64];
        public ParameterSetsStruc[] parameterForm2 = new ParameterSetsStruc[64];
        // f�r PIC Register 1 und 2
        public ParameterSetsStruc[] parameterPic1 = new ParameterSetsStruc[64];
        public ParameterSetsStruc[] parameterPic2 = new ParameterSetsStruc[64];

        //namen der Parameter f�r kurz-Ausgabe
        //TODO: Array erweitern wenn neue Parameter hinzukommen
        string[] kurzParameter = { "RP","RI","RD","RL","RIL",
                                    "\r\nNP","NI","ND","NL","NIL",
                                    "\r\nGP","GI","GD","GL","GIL",
                                    "\r\nGK","IA","AU","LRL","LVZ",
                                    "\r\nLAA","LWG","LL","LWR","LWN",
                                    "\r\nKRN","KO", "BD", "Radius", "Wind",
                                    "\r\nNAP", "AI", "Alt", "MVar", "Gyro", "ESC", "TxRx",
                                    "CRadius", "u39",
                                    "u40","u41","u42","u43","u44","u45","u46","u47","u48","u49",
                                    "u50","u51","u52","u53","u54","u55","u56","u57","u58","u59",
                                    "u60","u61","u62","u63","u64"};
  
        /// <summary>
        /// Laden einer Konfigurationsdatei
        /// </summary>
        /// <param name="pfad"></param>
        /// <param name="mainForm"></param>
        public void laden(string pfad, FormMain mainForm)
        {
            // try / catch wenn fehler beim �ffnen oder interpretieren der datei
            try 
            {
                IniReader iniReader = new IniReader(pfad);
                // log schreiben bei debug
                Log.write(mainForm, "Load Parameterset: " + pfad, 1);
                ParameterSetsStruc[] registers = iniReader.GetChapter("ChannelSet");
                // alle Felder auf rot setzen
                updateForm(registers, mainForm);
            } 
            catch (Exception e) 
            {
                MessageBox.Show(e.ToString(), "Error!", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Log.write(mainForm, "Wrong Parameter File!", 0);
            }
        }

        /// <summary>
        /// speichern der Parameter in eine Datei
        /// </summary>
        /// <param name="pfad"></param>
        /// <param name="mainForm"></param>
        public void speichern(string pfad, FormMain mainForm)
        {

            StreamWriter sw = new StreamWriter(pfad, false, Encoding.GetEncoding("windows-1252")); 
            try 
            {
                // header f�r Datei
                sw.WriteLine("[ChannelSet]");
                // es wird immer das aktive Tab gespeichert
                if (mainForm.tabControlParameter.SelectedIndex == 0)
                {
                    foreach (ParameterSetsStruc register in parameterForm1)
                        if (register.Command != null)
                            sw.WriteLine(register.Command + "=" + register.Value);
                }
                else
                {
                    foreach (ParameterSetsStruc register in parameterForm2)
                        if (register.Command != null)
                            sw.WriteLine(register.Command + "=" + register.Value);
                }
            } 
            catch (Exception e) 
            {
                MessageBox.Show(e.ToString(), "Error!", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Log.write(mainForm, e.ToString(), 1);
            } 
            finally 
            {
                // datei schlie�en
                sw.Close();
            }
            // log schreiben bei debug
            Log.write(mainForm, "Write Parameterset: " + pfad, 1);
        }

        /// <summary>
        /// ein einzelnes Feld updaten
        /// </summary>
        /// <param name="objekt"></param>
        /// <param name="mainForm"></param>
        public void feldUpdaten(Object objekt, FormMain mainForm)
        {
            // wenn es sich um ein Parameter feld handelt
            if (objekt.GetType().Name == "NumericUpDown") 
            {
                NumericUpDown feld = (NumericUpDown)objekt;
                // je nach Parameter TAB
                if (mainForm.tabControlParameter.SelectedIndex == 0) 
                {
                    //feld nur setzen wenn Integral Limit OK ist - sonst return ohne update
                    if (feld.Tag.ToString() == "5" || feld.Tag.ToString() == "10" || feld.Tag.ToString() == "15")
                    {
                        //integral Limit errechnen
                        int tempValue = Convert.ToInt32(parameterForm1[Convert.ToInt16(feld.Tag) - 4].Value);
                        if (tempValue < 0)
                            tempValue = tempValue * -1;
                        int limitTemp = (int)Math.Floor(127.0 / Convert.ToDouble(tempValue));
                        
                        if (limitTemp < Convert.ToInt32(feld.Value))
                        {
                            if(limitTemp >= feld.Minimum && limitTemp <= feld.Maximum)
                                feld.Value = Convert.ToDecimal(parameterForm1[Convert.ToInt16(feld.Tag) - 1].Value);
 
                            return;
                        }  
                    }

                    parameterForm1[Convert.ToInt16(feld.Tag)-1].Value = feld.Value.ToString();
                    
                    //IntegralLimit berechnen wenn Integral ge�ndert wurde
                    //Integral limit compensation - reduces limit for larger Ki to keep within 16 bits
                    if (feld.Tag.ToString() == "2" || feld.Tag.ToString() == "7" || feld.Tag.ToString() == "12")
                    {
                        //integral Limit errechnen
                        int tempValue = Convert.ToInt32(parameterForm1[Convert.ToInt16(feld.Tag) - 1].Value);
                        if (tempValue < 0)
                            tempValue = tempValue * -1;
                        int limitTemp = (int) Math.Floor(127.0 / Convert.ToDouble(tempValue));
                        
                        if (limitTemp > 0 && limitTemp < Convert.ToInt32(parameterForm1[Convert.ToInt16(feld.Tag) + 2].Value))
                        { 
                            switch (feld.Tag.ToString())
                            {
                                case "2":
                                    mainForm.RollIntLimit1NumericUpDown.Value = limitTemp;
                                    break;
                                case "7":
                                    mainForm.PitchIntLimit1NumericUpDown.Value = limitTemp;
                                    break;
                                case "12":
                                    mainForm.YawIntLimit1NumericUpDown.Value = limitTemp;
                                    break;
                            }
                        }
                    }
                     
                    // wenn es gleich dem PIC wert ist -> wieder gr�n
                    //Hier die Idee der Farbdarstellung:
                    //schwarz: wenn noch keine Daten gelesen wurden (dh nicht so wie jetzt wo die Werte ja rot werden und beim Initialwert gr�n)
                    //orange: wenn Pic schon gelesen wurde und die Werte ungeleich den Werten des PICs sind
                    //gr�n: wenn Pic gelesen wurde und die Werte gleich dem PIC sind
                    //rot: wenn Pic geschrieben wurde und die Werte nach nochmaligem lesen nicht gleich sind
                    
                    // wenn pic noch nicht gelesen wurde ist Chapter leer
                    if (parameterPic1[Convert.ToInt16(feld.Tag) - 1].Chapter == "ChannelSet")
                        // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                        if (parameterForm1[Convert.ToInt16(feld.Tag) - 1].Value ==
                            parameterPic1[Convert.ToInt16(feld.Tag) - 1].Value)
                            feld.ForeColor = Color.Green;
                        // wenn wert nicht mit PIC ident
                        else
                            // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                            if (mainForm.writeUpdate == true)
                                feld.ForeColor = Color.Red;
                            else
                                feld.ForeColor = Color.Orange;
                    else
                    // dann wird feld auf black gesetzt
                        feld.ForeColor = Color.Black;
                } 
                else
                {
                    //feld nur setzen wenn Integral Limit OK ist - sonst return ohne update
                    if (feld.Tag.ToString() == "5" || feld.Tag.ToString() == "10" || feld.Tag.ToString() == "15")
                    {
                        //integral Limit errechnen
                        int tempValue = Convert.ToInt32(parameterForm2[Convert.ToInt16(feld.Tag) - 4].Value);
                        if (tempValue < 0)
                            tempValue = tempValue * -1;
                        int limitTemp = (int)Math.Floor(127.0 / Convert.ToDouble(tempValue));
                        if (limitTemp < Convert.ToInt32(feld.Value))
                        {
                            feld.Value = Convert.ToDecimal(parameterForm2[Convert.ToInt16(feld.Tag) - 1].Value);
                            return;
                        }

                    }

                    parameterForm2[Convert.ToInt16(feld.Tag)-1].Value = feld.Value.ToString();

                    //IntegralLimit berechnen wenn Integral ge�ndert wurde
                    //Integral limit compensation - reduces limit for larger Ki to keep within 16 bits
                    if (feld.Tag.ToString() == "2" || feld.Tag.ToString() == "7" || feld.Tag.ToString() == "12")
                    {
                        //integral Limit errechnen

                        int tempValue = Convert.ToInt32(parameterForm2[Convert.ToInt16(feld.Tag) - 1].Value);
                        if (tempValue < 0)
                            tempValue = tempValue * -1;
                        int limitTemp = (int)Math.Floor(127.0 / Convert.ToDouble(tempValue));
                        
                        if (limitTemp > 0 && limitTemp < Convert.ToInt32(parameterForm2[Convert.ToInt16(feld.Tag) + 2].Value))
                            switch (feld.Tag.ToString())
                            {
                                case "2":
                                    mainForm.RollIntLimit2NumericUpDown.Value = limitTemp;
                                    break;
                                case "7":
                                    mainForm.PitchIntLimit2NumericUpDown.Value = limitTemp;
                                    break;
                                case "12":
                                    mainForm.YawIntLimit2NumericUpDown.Value = limitTemp;
                                    break;
                            }
                    }
                                      
                    // wenn pic noch nicht gelesen wurde ist Chapter leer
                    if (parameterPic2[Convert.ToInt16(feld.Tag) - 1].Chapter == "ChannelSet")
                        // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                        if (parameterForm2[Convert.ToInt16(feld.Tag) - 1].Value ==
                            parameterPic2[Convert.ToInt16(feld.Tag) - 1].Value)
                            feld.ForeColor = Color.Green;
                        // wenn wert nicht mit PIC ident
                        else
                            // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                            if (mainForm.writeUpdate == true)
                                feld.ForeColor = Color.Red;
                            else 
                                feld.ForeColor = Color.Orange;
                    else
                        // dann wird feld auf black gesetzt
                        feld.ForeColor = Color.Black;
                }
            }
            // wenn es ein BIT Wert ist muss der Parameter Wert errechnet werden
            else
                if (objekt.GetType().Name == "Button")
                {
                    Button feld = (Button)objekt;

                    // welches BIT ist betroffen?
                    switch (feld.Name)
                    {
                        case "Sense01Button": // Throttle
                            if (mainForm.writeUpdate == false)
                                if (mainForm.SenseButton[0])
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) | 1).ToString();
                                    feld.BackColor = Color.LightGray;
                                }
                                else
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) & 254).ToString();
                                    feld.BackColor = Color.White;
                                }

                            // wenn pic noch nicht gelesen wurde ist Chapter leer
                            if (parameterPic1[51].Chapter == "ChannelSet")
                                // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                                if ((Convert.ToInt16(parameterForm1[51].Value) & 1) ==
                                    (Convert.ToInt16(parameterPic1[51].Value) & 1))
                                    feld.ForeColor = Color.Green;
                                // wenn wert nicht mit PIC ident
                                else
                                    // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                    if (mainForm.writeUpdate == true)
                                        feld.ForeColor = Color.Red;
                                    else
                                        feld.ForeColor = Color.Orange;
                            else
                                // dann wird feld auf black gesetzt
                                feld.ForeColor = Color.Black;

                            break;
                        case "Sense11Button":
                            if (mainForm.writeUpdate == false)
                                if (mainForm.SenseButton[1])
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) | 2).ToString();
                                    feld.BackColor = Color.LightGray;
                                }
                                else
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) & 253).ToString();
                                    feld.BackColor = Color.White;
                                }
                            // wenn pic noch nicht gelesen wurde ist Chapter leer
                            if (parameterPic1[51].Chapter == "ChannelSet")
                                // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                                if ((Convert.ToInt16(parameterForm1[51].Value) & 2) ==
                                    (Convert.ToInt16(parameterPic1[51].Value) & 2))
                                    feld.ForeColor = Color.Green;
                                // wenn wert nicht mit PIC ident
                                else
                                    // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                    if (mainForm.writeUpdate == true)
                                        feld.ForeColor = Color.Red;
                                    else
                                        feld.ForeColor = Color.Orange;
                            else
                                // dann wird feld auf black gesetzt
                                feld.ForeColor = Color.Black;

                            break;
                        case "Sense21Button":
                            if (mainForm.writeUpdate == false)
                                if (mainForm.SenseButton[2])
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) | 4).ToString();
                                    feld.BackColor = Color.LightGray;
                                }
                                else
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) & 251).ToString();
                                    feld.BackColor = Color.White;
                                }
                            // wenn pic noch nicht gelesen wurde ist Chapter leer
                            if (parameterPic1[51].Chapter == "ChannelSet")
                                // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                                if ((Convert.ToInt16(parameterForm1[51].Value) & 4) ==
                                    (Convert.ToInt16(parameterPic1[51].Value) & 4))
                                    feld.ForeColor = Color.Green;
                                // wenn wert nicht mit PIC ident
                                else
                                    // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                    if (mainForm.writeUpdate == true)
                                        feld.ForeColor = Color.Red;
                                    else
                                        feld.ForeColor = Color.Orange;
                            else
                                // dann wird feld auf black gesetzt
                                feld.ForeColor = Color.Black;

                            break;
                        case "Sense31Button":
                            if (mainForm.writeUpdate == false)
                                if (mainForm.SenseButton[3])
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) | 8).ToString();
                                    feld.BackColor = Color.LightGray;
                                }
                                else
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) & 247).ToString();
                                    feld.BackColor = Color.White;
                                }
                            // wenn pic noch nicht gelesen wurde ist Chapter leer
                            if (parameterPic1[51].Chapter == "ChannelSet")
                                // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                                if ((Convert.ToInt16(parameterForm1[51].Value) & 8) ==
                                    (Convert.ToInt16(parameterPic1[51].Value) & 8))
                                    feld.ForeColor = Color.Green;
                                // wenn wert nicht mit PIC ident
                                else
                                    // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                    if (mainForm.writeUpdate == true)
                                        feld.ForeColor = Color.Red;
                                    else
                                        feld.ForeColor = Color.Orange;
                            else
                                // dann wird feld auf black gesetzt
                                feld.ForeColor = Color.Black;

                            break;

                        case "Sense41Button":
                            if (mainForm.writeUpdate == false)
                                if (mainForm.SenseButton[4])
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) | 16).ToString();
                                    feld.BackColor = Color.LightGray;
                                }
                                else
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) & 239).ToString();
                                    feld.BackColor = Color.White;
                                }
                            // wenn pic noch nicht gelesen wurde ist Chapter leer
                            if (parameterPic1[51].Chapter == "ChannelSet")
                                // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                                if ((Convert.ToInt16(parameterForm1[51].Value) & 16) ==
                                    (Convert.ToInt16(parameterPic1[51].Value) & 16))
                                    feld.ForeColor = Color.Green;
                                // wenn wert nicht mit PIC ident
                                else
                                    // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                    if (mainForm.writeUpdate == true)
                                        feld.ForeColor = Color.Red;
                                    else
                                        feld.ForeColor = Color.Orange;
                            else
                                // dann wird feld auf black gesetzt
                                feld.ForeColor = Color.Black;
                            break;

                        case "Sense51Button":
                            if (mainForm.writeUpdate == false)
                                if ( mainForm.SenseButton[5] )
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) | 32).ToString();
                                    feld.BackColor = Color.LightGray;
                                }
                                else
                                {
                                    parameterForm1[51].Value = (Convert.ToInt16(parameterForm1[51].Value) & 223).ToString();
                                    feld.BackColor = Color.White;
                                }

                            // wenn pic noch nicht gelesen wurde ist Chapter leer
                            if (parameterPic1[51].Chapter == "ChannelSet")
                                // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                                if ((Convert.ToInt16(parameterForm1[51].Value) & 32) ==
                                    (Convert.ToInt16(parameterPic1[51].Value) & 32))
                                    feld.ForeColor = Color.Green;
                                // wenn wert nicht mit PIC ident
                                else
                                    // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                    if (mainForm.writeUpdate == true)
                                        feld.ForeColor = Color.Red;
                                    else
                                        feld.ForeColor = Color.Orange;
                            else
                                // dann wird feld auf black gesetzt
                                feld.ForeColor = Color.Black;
                            break;
                    }
                }
                else
                if (objekt.GetType().Name == "CheckBox")
                {
                CheckBox feld = (CheckBox)objekt;

                // welches BIT ist betroffen?
                switch (feld.Name) 
                {
                    case "bit01CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 1).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 254).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 1) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 1))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;

                        break;
                    case "bit11CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 2).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 253).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 2) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 2))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;

                        break;
                    case "bit21CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 4).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 251).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 4) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 4))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;

                        break;
                    case "bit31CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 8).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 247).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 8) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 8))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;

                        break;
                    case "bit41CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 16).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 239).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 16) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 16))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit51CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 32).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 223).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 32) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 32))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit61CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) | 64).ToString();
                            else
                                parameterForm1[15].Value = (Convert.ToInt16(parameterForm1[15].Value) & 191).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic1[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm1[15].Value) & 64) ==
                                (Convert.ToInt16(parameterPic1[15].Value) & 64))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit02CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 1).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 254).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 1) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 1))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit12CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 2).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 253).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 2) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 2))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit22CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 4).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 251).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 4) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 4))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit32CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 8).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 247).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 8) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 8))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                        // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit42CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 16).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 239).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 16) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 16))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;

                    case "bit52CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 32).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 223).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 32) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 32))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;
                    case "bit62CheckBox":
                        if (mainForm.writeUpdate == false)
                            if (feld.Checked == true)
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) | 64).ToString();
                            else
                                parameterForm2[15].Value = (Convert.ToInt16(parameterForm2[15].Value) & 191).ToString();

                        // wenn pic noch nicht gelesen wurde ist Chapter leer
                        if (parameterPic2[15].Chapter == "ChannelSet")
                            // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                            if ((Convert.ToInt16(parameterForm2[15].Value) & 64) ==
                                (Convert.ToInt16(parameterPic2[15].Value) & 64))
                                feld.ForeColor = Color.Green;
                            // wenn wert nicht mit PIC ident
                            else
                                // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                                if (mainForm.writeUpdate == true)
                                    feld.ForeColor = Color.Red;
                                else
                                    feld.ForeColor = Color.Orange;
                        else
                            // dann wird feld auf black gesetzt
                            feld.ForeColor = Color.Black;
                        break;
                }
                

            }
            else
            {
                ComboBox feld = (ComboBox)objekt;

                if (mainForm.tabControlParameter.SelectedIndex == 0)
                {
                    parameterForm1[Convert.ToInt16(feld.Tag) - 1].Value = feld.SelectedIndex.ToString();
                    if (parameterPic1[Convert.ToInt16(feld.Tag) - 1].Chapter == "ChannelSet")
                        // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                        if (parameterForm1[Convert.ToInt16(feld.Tag) - 1].Value ==
                            parameterPic1[Convert.ToInt16(feld.Tag) - 1].Value)
                            feld.ForeColor = Color.Green;
                        // wenn wert nicht mit PIC ident
                        else
                            // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                            if (mainForm.writeUpdate == true)
                                feld.ForeColor = Color.Red;
                            else
                                feld.ForeColor = Color.Orange;
                    else
                        // dann wird feld auf black gesetzt
                        feld.ForeColor = Color.Black;
                }
                else
                {
                    parameterForm2[Convert.ToInt16(feld.Tag) - 1].Value = feld.SelectedIndex.ToString();
                    if (parameterPic2[Convert.ToInt16(feld.Tag) - 1].Chapter == "ChannelSet")
                        // wenn pic schon gelesen wurde ist ein Vergleich notwendig
                        if (parameterForm2[Convert.ToInt16(feld.Tag) - 1].Value ==
                            parameterPic2[Convert.ToInt16(feld.Tag) - 1].Value)
                            feld.ForeColor = Color.Green;
                        // wenn wert nicht mit PIC ident
                        else
                            // wird das Feld beim schreiben upgedatet dann ist das Feld bei fehler rot
                            if (mainForm.writeUpdate == true)
                                feld.ForeColor = Color.Red;
                            else
                                feld.ForeColor = Color.Orange;
                    else
                        // dann wird feld auf black gesetzt
                        feld.ForeColor = Color.Black;
                }
            }
        }

        /// <summary>
        /// Update der Felder in der Form
        /// </summary>
        /// <param name="registers">Structur der Registerwerte</param>
        ///// <param name="mainForm">Mainform</param>
        public void updateForm(ParameterSetsStruc[] registers, FormMain mainForm)
        {

            farbenSetzen(mainForm, Farbe.black);

            //TODO: hier case f�r neuen Parameter hinzuf�gen
            // alle Parameter durchgehen und lt. array setzen
            foreach (ParameterSetsStruc register in registers) 
            {
                if (mainForm.tabControlParameter.SelectedIndex == 0) 
                {
                    switch (Convert.ToInt16(register.Command.Substring(8))) 
                    {
                        case 1:
                            parameterForm1[0].Value = register.Value;
                            mainForm.RollProp1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.RollProp1NumericUpDown, mainForm);
                            break;
                        case 2:
                            parameterForm1[1].Value = register.Value;
                            mainForm.RollInt1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.RollInt1NumericUpDown, mainForm);
                            break;
                        case 3:
                            parameterForm1[2].Value = register.Value;
                            mainForm.RollDiff1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.RollDiff1NumericUpDown, mainForm);
                            break;
                        case 4:
                            mainForm.HorizDamping1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            parameterForm1[3].Value = register.Value;
                            feldUpdaten(mainForm.HorizDamping1NumericUpDown, mainForm);
                            break;
                        case 5:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm1[4].Value = register.Value;
                                mainForm.RollIntLimit1NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm1[4].Value = register.Value;
                                mainForm.RollIntLimit1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.RollIntLimit1NumericUpDown, mainForm);
                            break;
                        case 6:
                            parameterForm1[5].Value = register.Value;
                            mainForm.PitchProp1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchProp1NumericUpDown, mainForm);
                            break;
                        case 7:
                            parameterForm1[6].Value = register.Value;
                            mainForm.PitchInt1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchInt1NumericUpDown, mainForm);
                            break;
                        case 8:
                            parameterForm1[7].Value = register.Value;
                            mainForm.PitchDiff1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchDiff1NumericUpDown, mainForm);
                            break;
                        case 9:
                            mainForm.AltProp1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            parameterForm1[8].Value = register.Value;
                            feldUpdaten(mainForm.AltProp1NumericUpDown, mainForm);
                            break;
                        case 10:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm1[9].Value = register.Value;
                                mainForm.PitchIntLimit1NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm1[9].Value = register.Value;
                                mainForm.PitchIntLimit1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.PitchIntLimit1NumericUpDown, mainForm);
                            break;
                        case 11:
                            parameterForm1[10].Value = register.Value;
                            mainForm.YawProp1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.YawProp1NumericUpDown, mainForm);
                            break;
                        case 12:
                            parameterForm1[11].Value = register.Value;
                            mainForm.YawInt1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.YawInt1NumericUpDown, mainForm);
                            break;
                        case 13:
                            parameterForm1[12].Value = register.Value;
                            mainForm.YawDiff1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.YawDiff1NumericUpDown, mainForm);
                            break;
                        case 14:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm1[13].Value = register.Value;
                                mainForm.YawLimit1NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm1[13].Value = register.Value;
                                mainForm.YawLimit1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.YawLimit1NumericUpDown, mainForm);
                            break;
                        case 15:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm1[14].Value = register.Value;
                                mainForm.YawIntLimit1NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm1[14].Value = register.Value;
                                mainForm.YawIntLimit1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.YawIntLimit1NumericUpDown, mainForm);
                            break;
                        case 16:
                            int temp = Convert.ToInt16(register.Value);
                            if (temp == -1)
                                temp = 0;

                            if ((temp & 1) == 1) 
                            {
                                mainForm.bit01CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit01CheckBox, mainForm);
                            } 
                            else 
                            {
                                mainForm.bit01CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit01CheckBox, mainForm);
                            }
                            if ((temp & 2) == 2) 
                            {
                                mainForm.bit11CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit11CheckBox, mainForm);
                            } 
                            else 
                            {
                                mainForm.bit11CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit11CheckBox, mainForm);
                            }
                            if ((temp & 4) == 4)
                            {
                                mainForm.bit21CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit21CheckBox, mainForm);
                            }
                            else
                            {
                                mainForm.bit21CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit21CheckBox, mainForm);
                            }
                            if ((temp & 8) == 8) 
                            {
                                mainForm.bit31CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit31CheckBox, mainForm);
                            } 
                            else 
                            {
                                mainForm.bit31CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit31CheckBox, mainForm);
                            }
                            if ((temp & 16) == 16) 
                            {
                                mainForm.bit41CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit41CheckBox, mainForm);
                            } 
                            else 
                            {
                                mainForm.bit41CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit41CheckBox, mainForm);
                            }
                            if ((temp & 32) == 32)
                            {
                                mainForm.bit51CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit51CheckBox, mainForm);
                            }
                            else
                            {
                                mainForm.bit51CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit51CheckBox, mainForm);
                            }
                            if ((temp & 64) == 64)
                            {
                                mainForm.bit61CheckBox.Checked = true;
                                feldUpdaten(mainForm.bit61CheckBox, mainForm);
                            }
                            else
                            {
                                mainForm.bit61CheckBox.Checked = false;
                                feldUpdaten(mainForm.bit61CheckBox, mainForm);
                            }
                            //parameterForm1[15].Value = temp.ToString();
                            break;
                        case 17:
                            parameterForm1[16].Value = register.Value;
                            mainForm.ImpulseTime1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.ImpulseTime1NumericUpDown, mainForm);
                            break;
                        case 18:
                            if (Convert.ToInt16(register.Value) < 0)
                            {
                                parameterForm1[17].Value = register.Value;
                                mainForm.Battery1NumericUpDown.Value = 0;
                            }
                            else
                            {
                                parameterForm1[17].Value = register.Value;
                                mainForm.Battery1NumericUpDown.Value = Convert.ToDecimal(Math.Round(Convert.ToDouble(register.Value), 1, MidpointRounding.AwayFromZero));
                            }
                            feldUpdaten(mainForm.Battery1NumericUpDown, mainForm);
                            break;
                        case 19:
                            parameterForm1[18].Value = register.Value;
                            mainForm.CameraRoll1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CameraRoll1NumericUpDown, mainForm);
                            break;
                        case 20:
                            parameterForm1[19].Value = register.Value;
                            mainForm.HoverThrottle1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.HoverThrottle1NumericUpDown, mainForm);
                            break;
                        case 21:
                            parameterForm1[20].Value = register.Value;
                            mainForm.VertDamping1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.VertDamping1NumericUpDown, mainForm);
                            break;
                        case 22:
                            parameterForm1[21].Value = register.Value;
                            mainForm.VertAcc1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.VertAcc1NumericUpDown, mainForm);
                            break;
                        case 23:
                            if (Convert.ToInt16(register.Value) < 5) 
                            {
                                parameterForm1[22].Value = register.Value;
                                mainForm.LowMotorRun1NumericUpDown.Value = 5;
                            } 
                            else 
                            {
                                parameterForm1[22].Value = register.Value;
                                mainForm.LowMotorRun1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.LowMotorRun1NumericUpDown, mainForm);
                            break;
                        case 24:
                            parameterForm1[23].Value = register.Value;
                            mainForm.RollAcc1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.RollAcc1NumericUpDown, mainForm);
                            break;
                        case 25:
                            parameterForm1[24].Value = register.Value;
                            mainForm.PitchAcc1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchAcc1NumericUpDown, mainForm);
                            break;
                        case 26:
                            parameterForm1[25].Value = register.Value;
                            mainForm.CameraPitch1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CameraPitch1NumericUpDown, mainForm);
                            break;
                        case 27:
                            parameterForm1[26].Value = register.Value;
                            mainForm.Compass1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.Compass1NumericUpDown, mainForm);
                            break;
                        case 28:
                            parameterForm1[27].Value = register.Value;
                            mainForm.AltInt1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltInt1NumericUpDown, mainForm);
                            break;
                        case 29:
                            parameterForm1[28].Value = register.Value;
                            mainForm.NavRadius1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavRadius1NumericUpDown, mainForm);
                            break;
                        case 30:
                            parameterForm1[29].Value = register.Value;
                            mainForm.NavInt1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavInt1NumericUpDown, mainForm);
                            break;
                        case 31:
                            break;
                        case 32:
                            break;
                        case 33:
                            parameterForm1[32].Value = register.Value;
                            mainForm.NavRTHAlt1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavRTHAlt1NumericUpDown, mainForm);
                            break;
                        case 34:
                            parameterForm1[33].Value = register.Value;
                            mainForm.NavMagVar1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavMagVar1NumericUpDown, mainForm);
                            break;
                        case 35:
                            parameterForm1[34].Value = register.Value;
                            mainForm.GyroRollPitchComboBox1.SelectedIndex = Convert.ToInt16(register.Value);  
                            feldUpdaten(mainForm.GyroRollPitchComboBox1, mainForm);
                            break;
                        case 36:
                            parameterForm1[35].Value = register.Value;
                            mainForm.ESCComboBox1.SelectedIndex = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.ESCComboBox1, mainForm);
                            break;
                        case 37:
                            parameterForm1[36].Value = register.Value;
                            mainForm.TxRxComboBox1.SelectedIndex = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.TxRxComboBox1, mainForm);
                            break;
                        case 38:
                            parameterForm1[37].Value = register.Value;
                            mainForm.NavNeutralRadius1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavNeutralRadius1NumericUpDown, mainForm);
                            break;
                        case 39:
                            parameterForm1[38].Value = register.Value;
                            mainForm.GPSGain1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.GPSGain1NumericUpDown, mainForm);
                            break;
                        case 40:
                            parameterForm1[39].Value = register.Value;
                            mainForm.CameraRollTrim1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CameraRollTrim1NumericUpDown, mainForm);
                            break;
                        case 41:
                            parameterForm1[40].Value = register.Value;
                            mainForm.NavDiff1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavDiff1NumericUpDown, mainForm);
                            break;
                        case 42:
                            parameterForm1[41].Value = register.Value;
                            mainForm.VertDampingDecay1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.VertDampingDecay1NumericUpDown, mainForm);
                            break;
                        case 43:
                            parameterForm1[42].Value = register.Value;
                            mainForm.HorizDampingDecay1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.HorizDampingDecay1NumericUpDown, mainForm);
                            break;
                        case 44:
                            parameterForm1[43].Value = register.Value;
                            mainForm.BaroScale1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.BaroScale1NumericUpDown, mainForm);
                            break;
                        case 45:
                            parameterForm1[44].Value = register.Value;
                            mainForm.TelemetryComboBox1.SelectedIndex = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.TelemetryComboBox1, mainForm);
                            break;
                        case 46:
                            parameterForm1[45].Value = register.Value;
                            mainForm.DescentRate1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.DescentRate1NumericUpDown, mainForm);
                            break;
                        case 47:
                            parameterForm1[46].Value = register.Value;
                            mainForm.DescDelay1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.DescDelay1NumericUpDown, mainForm);
                            break;
                        case 48:
                            parameterForm1[47].Value = register.Value;
                            mainForm.NavIntLimit1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavIntLimit1NumericUpDown, mainForm);
                            break;
                        case 49:
                            parameterForm1[48].Value = register.Value;
                            mainForm.AltIntLimit1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltIntLimit1NumericUpDown, mainForm);
                            break;
                        case 50:
                            parameterForm1[49].Value = register.Value;
                            mainForm.GravComp1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.GravComp1NumericUpDown, mainForm);
                            break;
                        case 51:
                            parameterForm1[50].Value = register.Value;
                            mainForm.CompSteps1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CompSteps1NumericUpDown, mainForm);
                            break;
                        case 52:
                            parameterForm1[51].Value = register.Value;
                            temp = Convert.ToInt16(register.Value);

                            mainForm.SenseButton[0] = (temp & 1) != 0;
                            mainForm.SenseButton[1] = (temp & 2) != 0;
                            mainForm.SenseButton[2] = (temp & 4) != 0;
                            mainForm.SenseButton[3] = (temp & 8) != 0;
                            mainForm.SenseButton[4] = (temp & 16) != 0;
                            mainForm.SenseButton[5] = (temp & 32) != 0;

                            feldUpdaten(mainForm.Sense01Button, mainForm);
                            feldUpdaten(mainForm.Sense11Button, mainForm);
                            feldUpdaten(mainForm.Sense21Button, mainForm);
                            feldUpdaten(mainForm.Sense31Button, mainForm);
                            feldUpdaten(mainForm.Sense41Button, mainForm);
                            feldUpdaten(mainForm.Sense51Button, mainForm);
                            break;
                        case 53:
                            parameterForm1[52].Value = register.Value;
                            mainForm.CompassOffset1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CompassOffset1NumericUpDown, mainForm);
                            break;
                        case 54:
                            parameterForm1[53].Value = register.Value;
                            mainForm.BatteryCapacity1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.BatteryCapacity1NumericUpDown, mainForm);
                            break;
                        case 55:
                            parameterForm1[54].Value = register.Value;
                            mainForm.GyroYawComboBox1.SelectedIndex = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.GyroYawComboBox1, mainForm);
                            break;
                        case 56:
                            parameterForm1[55].Value = register.Value;
                            mainForm.AltKd1NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltKd1NumericUpDown, mainForm);
                            break;
                        
                        default: break; // up to case 64 available
                    }
                } 
                else 
                {
                    switch (Convert.ToInt16(register.Command.Substring(9))) 
                    {
                        case 1:
                            mainForm.RollProp2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            parameterForm2[0].Value = register.Value;
                            feldUpdaten(mainForm.RollProp2NumericUpDown, mainForm);
                            break;
                        case 2:
                            parameterForm2[1].Value = register.Value;
                            mainForm.RollInt2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.RollInt2NumericUpDown, mainForm);
                            break;
                        case 3:
                            parameterForm2[2].Value = register.Value;
                            mainForm.RollDiff2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.RollDiff2NumericUpDown, mainForm);
                            break;

                        case 5:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm2[4].Value = register.Value;
                                mainForm.RollIntLimit2NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm2[4].Value = register.Value;
                                mainForm.RollIntLimit2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.RollIntLimit2NumericUpDown, mainForm);
                            break;
                        case 6:
                            parameterForm2[5].Value = register.Value;
                            mainForm.PitchProp2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchProp2NumericUpDown, mainForm);
                            break;
                        case 7:
                            parameterForm2[6].Value = register.Value;
                            mainForm.PitchInt2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchInt2NumericUpDown, mainForm);
                            break;
                        case 8:
                            parameterForm2[7].Value = register.Value;
                            mainForm.PitchDiff2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.PitchDiff2NumericUpDown, mainForm);
                            break;
                        case 9:
                            parameterForm2[8].Value = register.Value;
                            mainForm.AltProp2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltProp2NumericUpDown, mainForm);
                            break;
                        case 10:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm2[9].Value = register.Value;
                                mainForm.PitchIntLimit2NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm2[9].Value = register.Value;
                                mainForm.PitchIntLimit2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.PitchIntLimit2NumericUpDown, mainForm);
                            break;
                        case 11:
                            parameterForm2[10].Value = register.Value;
                            mainForm.YawProp2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.YawProp2NumericUpDown, mainForm);
                            break;
                        case 12:
                            parameterForm2[11].Value = register.Value;
                            mainForm.YawInt2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.YawInt2NumericUpDown, mainForm);
                            break;
                        case 13:
                            parameterForm2[12].Value = register.Value;
                            mainForm.YawDiff2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.YawDiff2NumericUpDown, mainForm);
                            break;
                        case 14:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                mainForm.YawLimit2NumericUpDown.Value = 1;
                                parameterForm2[13].Value = register.Value;
                            } 
                            else 
                            {
                                mainForm.YawLimit2NumericUpDown.Value = Convert.ToInt16(register.Value);
                                parameterForm2[13].Value = register.Value;
                            }
                            feldUpdaten(mainForm.YawLimit2NumericUpDown, mainForm);
                            break;
                        case 15:
                            if (Convert.ToInt16(register.Value) <= 0) 
                            {
                                parameterForm2[14].Value = register.Value;
                                mainForm.YawIntLimit2NumericUpDown.Value = 1;
                            } 
                            else 
                            {
                                parameterForm2[14].Value = register.Value;
                                mainForm.YawIntLimit2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            }
                            feldUpdaten(mainForm.YawIntLimit2NumericUpDown, mainForm);
                            break;
                        
                        case 19:
                            parameterForm2[18].Value = register.Value;
                            mainForm.CameraRoll2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CameraRoll2NumericUpDown, mainForm);
                            break;
                        
                        case 26:
                            parameterForm2[25].Value = register.Value;
                            mainForm.CameraPitch2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CameraPitch2NumericUpDown, mainForm);
                            break;
                        case 27:
                            parameterForm2[26].Value = register.Value;
                            mainForm.Compass2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.Compass2NumericUpDown, mainForm);
                            break;
                        case 28:
                            parameterForm2[27].Value = register.Value;
                            mainForm.AltInt2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltInt2NumericUpDown, mainForm);
                            break;
                        case 29:
                            parameterForm2[28].Value = register.Value;
                            mainForm.NavRadius2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavRadius2NumericUpDown, mainForm);
                            break;
                        case 30:
                            parameterForm2[29].Value = register.Value;
                            mainForm.NavInt2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavInt2NumericUpDown, mainForm);
                            break;
                        case 31:
                            break;
                        case 32:
                            break;
                        case 33:
                            parameterForm2[32].Value = register.Value;
                            mainForm.NavRTHAlt2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavRTHAlt2NumericUpDown, mainForm);
                            break;
                       
                        case 38:
                            parameterForm2[37].Value = register.Value;
                            mainForm.NavNeutralRadius2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavNeutralRadius2NumericUpDown, mainForm);
                            break;
                       
                        case 40:
                            parameterForm2[39].Value = register.Value;
                            mainForm.CameraRollTrim2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.CameraRollTrim2NumericUpDown, mainForm);
                            break;
                        case 41:
                            parameterForm2[40].Value = register.Value;
                            mainForm.NavDiff2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavDiff2NumericUpDown, mainForm);
                            break;
                        case 46:
                            parameterForm2[45].Value = register.Value;
                            mainForm.DescentRate2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.DescentRate2NumericUpDown, mainForm);
                            break;   
                        case 48:
                            parameterForm2[47].Value = register.Value;
                            mainForm.NavIntLimit2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.NavIntLimit2NumericUpDown, mainForm);
                            break;
                        case 49:
                            parameterForm2[48].Value = register.Value;
                            mainForm.AltIntLimit2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltIntLimit2NumericUpDown, mainForm);
                            break;
                        case 56:
                            parameterForm2[55].Value = register.Value;
                            mainForm.AltKd2NumericUpDown.Value = Convert.ToInt16(register.Value);
                            feldUpdaten(mainForm.AltKd2NumericUpDown, mainForm);
                            break;
                        
                        default: break; // up to case 64 available
                    }
                }
            }
        }


        /// <summary>
        /// fabe bei allen Feldern setzen
        /// </summary>
        /// <param name="mainForm"></param>
        /// <param name="farbe"></param>
        public void farbenSetzen(FormMain mainForm, Farbe farbe)
        { 
            
            Color farbeElement = Color.Black;
            switch (farbe) 
            { 
                case Farbe.red:
                   farbeElement = Color.Red;
                    break;
                case Farbe.green:
                    farbeElement = Color.Green;
                    break;
                case Farbe.orange:
                    farbeElement = Color.Orange;
                    break;
                case Farbe.black:
                    farbeElement = Color.Black;
                    break;
            }

            //TODO: hier Parameter hinzuf�gen wenn neu
            if (mainForm.tabControlParameter.SelectedIndex == 0) 
            {
                mainForm.RollProp1NumericUpDown.ForeColor = farbeElement;
                mainForm.RollInt1NumericUpDown.ForeColor = farbeElement;
                mainForm.RollDiff1NumericUpDown.ForeColor = farbeElement;
                mainForm.RollIntLimit1NumericUpDown.ForeColor = farbeElement;

                mainForm.PitchProp1NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchInt1NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchDiff1NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchIntLimit1NumericUpDown.ForeColor = farbeElement;

                mainForm.YawProp1NumericUpDown.ForeColor = farbeElement;
                mainForm.YawInt1NumericUpDown.ForeColor = farbeElement;
                mainForm.YawDiff1NumericUpDown.ForeColor = farbeElement;
                mainForm.YawLimit1NumericUpDown.ForeColor = farbeElement;
                mainForm.YawIntLimit1NumericUpDown.ForeColor = farbeElement;

                mainForm.bit01CheckBox.ForeColor = farbeElement;
                mainForm.bit61CheckBox.ForeColor = farbeElement;
                mainForm.bit21CheckBox.ForeColor = farbeElement;
                mainForm.bit31CheckBox.ForeColor = farbeElement;
                mainForm.bit41CheckBox.ForeColor = farbeElement;

                mainForm.ImpulseTime1NumericUpDown.ForeColor = farbeElement;

                mainForm.Battery1NumericUpDown.ForeColor = farbeElement;

                mainForm.VertDamping1NumericUpDown.ForeColor = farbeElement;
                mainForm.VertAcc1NumericUpDown.ForeColor = farbeElement;

                mainForm.LowMotorRun1NumericUpDown.ForeColor = farbeElement;

                mainForm.RollAcc1NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchAcc1NumericUpDown.ForeColor = farbeElement;
                mainForm.CameraPitch1NumericUpDown.ForeColor = farbeElement;
                mainForm.Compass1NumericUpDown.ForeColor = farbeElement;
                mainForm.AltInt1NumericUpDown.ForeColor = farbeElement;

                mainForm.NavRadius1NumericUpDown.ForeColor = farbeElement;
                mainForm.NavNeutralRadius1NumericUpDown.ForeColor = farbeElement;
                mainForm.NavInt1NumericUpDown.ForeColor = farbeElement;
                mainForm.NavRTHAlt1NumericUpDown.ForeColor = farbeElement;
                mainForm.NavMagVar1NumericUpDown.ForeColor = farbeElement;
            } 
            else 
            {
                mainForm.RollProp2NumericUpDown.ForeColor = farbeElement;
                mainForm.RollInt2NumericUpDown.ForeColor = farbeElement;
                mainForm.RollDiff2NumericUpDown.ForeColor = farbeElement;
                mainForm.RollIntLimit2NumericUpDown.ForeColor = farbeElement;

                mainForm.PitchProp2NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchInt2NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchDiff2NumericUpDown.ForeColor = farbeElement;
                mainForm.PitchIntLimit2NumericUpDown.ForeColor = farbeElement;

                mainForm.YawProp2NumericUpDown.ForeColor = farbeElement;
                mainForm.YawInt2NumericUpDown.ForeColor = farbeElement;
                mainForm.YawDiff2NumericUpDown.ForeColor = farbeElement;
                mainForm.YawLimit2NumericUpDown.ForeColor = farbeElement;
                mainForm.YawIntLimit2NumericUpDown.ForeColor = farbeElement;

              
                mainForm.CameraPitch2NumericUpDown.ForeColor = farbeElement;
                mainForm.Compass2NumericUpDown.ForeColor = farbeElement;
                mainForm.AltInt2NumericUpDown.ForeColor = farbeElement;

                mainForm.NavRadius2NumericUpDown.ForeColor = farbeElement;
                mainForm.NavNeutralRadius2NumericUpDown.ForeColor = farbeElement;
                mainForm.NavInt2NumericUpDown.ForeColor = farbeElement;
                mainForm.NavRTHAlt2NumericUpDown.ForeColor = farbeElement;

            }
        }

        public void copySet1ToSet2(FormMain mainForm)
        {
            mainForm.tabControlParameter.SelectedIndex = 1;
            updateForm(parameterForm1, mainForm);
        }

        public void copySet2ToSet1(FormMain mainForm)
        {
            mainForm.tabControlParameter.SelectedIndex = 0;
            updateForm(parameterForm2, mainForm);
        }
    }
}
