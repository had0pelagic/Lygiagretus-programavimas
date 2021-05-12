using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace L1
{
    class SortedResultMonitor
    {
        private static readonly object balanceLock = new object();
        private static int sizeArray = 0;
        public string[] array = null;
        public int size = 0;

        public void setSize(int size)
        {
            sizeArray = size;
            array = new string[sizeArray];
        }
        public void addStudent(string student, string hash)
        {
            lock (balanceLock)
            {
                string[] split = student.Split('|');
                this.array[size] = split[0] + "|" + split[1] + "|" + split[2] + "|" + hash;
                size++;
                for (int i = 0; i < size - 1; i++)
                {
                    for (int j = i + 1; j > 0; j--)
                    {
                        string[] fval = array[j - 1].Split('|');
                        string[] sval = array[j].Split('|');
                        if (Convert.ToInt32(fval[1]) > Convert.ToInt32(sval[1]))
                        {
                            string temp = array[j - 1];
                            array[j - 1] = array[j];
                            array[j] = temp;
                        }
                    }
                }
            }
        }
    }
}
