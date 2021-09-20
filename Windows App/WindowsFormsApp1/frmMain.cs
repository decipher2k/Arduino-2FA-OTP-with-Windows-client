using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using Microsoft.VisualBasic;
using System.Diagnostics;

namespace WindowsFormsApp1
{
    public partial class frmMain : Form
    {
        bool clickRequired = true;
        String status = "GETKEY";
        String password = "";
        String newPass = "";
        static int idx = 0;
        public frmMain()
        {
            InitializeComponent();
        }

        public delegate void SetTextDelegate(Control control, String text);
        public void SetText(Control control, String text)
        {
            if (control.InvokeRequired)
            {
                control.Invoke(new SetTextDelegate(SetText), new object[] { control, text });
            }
            else
            {
                control.Text = text;
            }
        }

        public delegate void AddKeyDelegate(Control control, Key key);
        public void AddKey(Control control, Key key)
        {
            if (control.InvokeRequired)
            {
                control.Invoke(new AddKeyDelegate(AddKey), new object[] { control, key });
            }
            else
            {
                ((ListBox)control).Items.Add(key);
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            string[] ports = SerialPort.GetPortNames();           
            foreach (string port in ports)
            {
                comboBox1.Items.Add(port);
            }
            serialPort1.Open();
            serialPort1.NewLine = "\n";
            serialPort1.DataReceived += SerialPort1_DataReceived;
            serialPort1.ErrorReceived += SerialPort1_ErrorReceived;

            string input = Interaction.InputBox("Password", "Password", "");
            if (input.Trim() != "")
            {
                password = input;
            }
            else
            {
                Application.Exit();
            }
        }

        private void SerialPort1_ErrorReceived(object sender, SerialErrorReceivedEventArgs e)
        {
            try
            {
                serialPort1.Close();
            }
            catch (Exception ex) { }
            try { 
                serialPort1.Open();
            }
            catch (Exception ex) { }
        }

        private void SerialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            SerialPort sp = (SerialPort)sender;
            String data = sp.ReadExisting();

            if (data.Contains("START"))
            {                
                if (status == "GETKEY")
                {
                    serialPort1.WriteLine(password);
                    serialPort1.WriteLine("GETKEY");
                    serialPort1.WriteLine(idx.ToString());

                    DateTime time = DateTime.Now;
                    long unixTime = ((DateTimeOffset)time).ToUnixTimeSeconds();
                    String ut = unixTime.ToString();
                    serialPort1.WriteLine(ut);
                }
                else if (status == "NEWPASS")
                {
                    serialPort1.WriteLine(password);
                    serialPort1.WriteLine("NEWPASS");
                    serialPort1.WriteLine(newPass);
                    status = "GETKEY";
                }
            }
            else if(data.Contains("WRONGPASS"))
            {
                MessageBox.Show("Password wrong.\nThe program will restart now.");
                Process.Start("WindowsFormsApp1.exe","restart");
                Application.Exit();

            }
            else
            {
                SetText(label2, "");
                MessageBox.Show(data);
            }
        }
        
        
        private void comboBox1_TextChanged(object sender, EventArgs e)
        {
            if(SerialPort.GetPortNames().Contains(comboBox1.Text))
            {
                try
                {
                    serialPort1.Close();
                }
                catch (Exception ex) { }
                try
                {
                    serialPort1.PortName = comboBox1.Text;
                    serialPort1.Open();
                }catch(Exception ex)
                {
                    MessageBox.Show("Can not open serial port.");
                }
            }
        }

        private void listBox1_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (!serialPort1.IsOpen)
            {
                MessageBox.Show("Please select a serial port.");
            }
            else
            {
                if (listBox1.SelectedIndex > -1)
                {
                    {
                        status = "GETKEY";
                        idx = (listBox1.SelectedIndex + 1);
                        if (!clickRequired)
                        {
                            serialPort1.WriteLine(password);
                            serialPort1.WriteLine("GETKEY");
                            serialPort1.WriteLine(idx.ToString());

                            DateTime time = DateTime.Now;
                            long unixTime = ((DateTimeOffset)time).ToUnixTimeSeconds();
                            String ut = unixTime.ToString();
                            serialPort1.WriteLine(ut);

                        }
                    }
                }
            }
        }

        private void addToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!serialPort1.IsOpen)
            {
                MessageBox.Show("Please select a serial port.");
            }
            else
            {
                if (password != "")
                {
                    status = "ADDKEY";
                    frmAddKey ak = new frmAddKey();
                    ak.ShowDialog();
                    Key k = new Key();
                    k.caption = ak.name;
                    AddKey(listBox1, k);
                    serialPort1.WriteLine(password);
                    serialPort1.WriteLine("ADDKEY");
                    serialPort1.WriteLine(ak.key.Length.ToString());
                    serialPort1.WriteLine(listBox1.Items.Count.ToString());
                    serialPort1.Write(Encoding.ASCII.GetBytes(ak.key), 0, ak.key.Length);
                    serialPort1.Write(new byte[] { (byte)'\n' }, 0, 1);
                    //serialPort1.WriteLine(ak.key);
                    status = "GETKEY";
                    SetText(label2, "Please click the Arduino button");
                }
            }
        }

        private void initializeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (!serialPort1.IsOpen)
            {
                MessageBox.Show("Please select a serial port.");
            }
            else
            {
                string input = Interaction.InputBox("Password", "Password", "");

                if (input.Trim() != "")
                {
                    password = input;
                    serialPort1.WriteLine(password);
                    serialPort1.WriteLine("INIT");
                    SetText(label2, "Please click the Arduino button");
                }
            }
        }

        private void resetPasswordToolStripMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show("Feature not yet implemented");
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }
    }    
}
