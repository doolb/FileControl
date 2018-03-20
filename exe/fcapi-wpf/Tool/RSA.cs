using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;


namespace Tool {

    public class RSA {

        public Key key;

        [StructLayout (LayoutKind.Sequential, CharSet=CharSet.Ansi)]
        public struct Key {
            public long M;
            public long D;
            public long E;

            public override string ToString () {
                return string.Format ("M:{0} D:{1} E:{2}", M, D, E);
            }
        }

        public void genKeys ( string file ) {

            // read primes number
            List<long> primes = new List<long> ();
            using (StreamReader sr = new StreamReader (file)) {
                long tmp = 0;
                while (!sr.EndOfStream) {
                    long.TryParse (sr.ReadLine (), out tmp);
                    if (tmp != 0) { primes.Add (tmp); }
                }
            }
            // choose random primes from the list, store them as p,q
            long p = 0;
            long q = 0;
            long e = 65537;
            long d = 0;
            long max = 0;
            long phi_max = 0;

            Random rand = new Random (DateTime.UtcNow.Millisecond);

            // find p q 
            do {
                int a = rand.Next (primes.Count);
                int b = rand.Next (primes.Count);

                p = primes[a];
                q = primes[b];

                max = p * q;
                phi_max = (p - 1)*(q - 1);
            }
            while (p == 0 || q==0 || p == q || phi_max.GCD (e) != 1);

            // find d
            d = RSA_EXT.Euclid (phi_max, e);
            while (d < 0) { d = d+phi_max; }

            // save value
            key.M = max;
            key.D = d;
            key.E = e;
        }

        public void import ( string file ) {

            using (StreamReader sr = new StreamReader (file)) {
                string[] ss = sr.ReadToEnd ().Split (',');
                if (ss.Length != 3)
                    throw new Exception ("RSA key file error");
                long.TryParse (ss[0], out key.M);
                long.TryParse (ss[1], out key.D);
                long.TryParse (ss[2], out key.E);
            }
        }

        public void export ( string file ) {
            using (StreamWriter sw = new StreamWriter (file)) {
                sw.Write (string.Format ("{0},{1},{2}", key.M, key.D, key.E));
            }
        }
    }

    public static class RSA_EXT {
        public static byte[] toBytes ( this string str ) {
            return Encoding.UTF8.GetBytes (str);
        }

        public static string toString ( this byte[] bytes ) {
            return Encoding.UTF8.GetString (bytes);
        }
        public static uint toUInt ( this byte[] bytes ) {
            uint r = 0;
            for (int i=0; i < bytes.Length && i < 4; i++) {
                r <<=8;
                r +=bytes[i];
            }
            return r;
        }
        public static long GCD ( this long a, long b ) {
            return b == 0 ? a : GCD (b, a % b);
        }
        public static long Euclid ( long a, long b ) {
            long x = 0, y = 1, u = 1, v = 0, gcd = b, m, n, q, r;
            while (a!=0) {
                q = gcd / a;
                r = gcd % a;
                m = x-u*q;
                n = y-v*q;
                gcd = a;
                a = r;
                x = u;
                y = v;
                u = m;
                v = n;
            }
            return y;
        }
    }
}