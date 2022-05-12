// #include <limits.h>
long long int llabs (long long int i)
{
  return i < 0 ? -i : i;
}

// double fabs(double x)
// {
//   return x < 0 ? -x : x; 
// }

// double frexp(double value, int *eptr)
// {
// 	union {
//                 double v;
//                 struct {
// 			unsigned u_mant2 : 32;
// 			unsigned u_mant1 : 20;
// 			unsigned   u_exp : 11;
//                         unsigned  u_sign :  1;
//                 } s;
//         } u;

// 	if (value) {
// 		u.v = value;
// 		*eptr = u.s.u_exp - 1022;
// 		u.s.u_exp = 1022;
// 		return(u.v);
// 	} else {
// 		*eptr = 0;
// 		return((double)0);
// 	}
// }

// double floor(double num)
// {
//     if (num >= LLONG_MAX || num <= LLONG_MIN || num != num) {
//         /* handle large values, infinities and nan */
//         return num;
//     }
//     long long n = (long long)num;
//     double d = (double)n;
//     if (d == num || num >= 0)
//         return d;
//     else
//         return d - 1;
// }