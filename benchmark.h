#include <time.h>
#include <cmath>
#include <iostream>
#include <tuple>
#include <functional>

struct clock_timer
{
  typedef clock_t time_t;
  time_t sample() { return clock(); }
  float difference(time_t t1, time_t t2) { return (t2-t1)/(float)CLOCKS_PER_SEC; }
};

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
struct gettimeofday_timer
{
  typedef timeval time_t;
  time_t sample() {
    timeval ret;
    gettimeofday(&ret, 0);
    return ret;
  }
  float difference(time_t t1, time_t t2) {
    float ret=(t2.tv_usec-t1.tv_usec)*1e-6;
    if(ret<0) {
      ret+=1;
      t2.tv_sec--;
    }
    ret+=t2.tv_sec-t1.tv_sec;
    return ret;
  }
};
#else
typedef clock_timer gettimeofday_timer;
#endif

namespace detail {
	void prepare(...) {}
}

template <class F, class Timer>
float run_bench(F f, float q, Timer tmr)
{
  using std::sqrt;
	using detail::prepare;
  float s=0, s2=0;
  for(unsigned n=1;; n++) {
		prepare(f);
    typename Timer::time_t t0=tmr.sample();
    f();
    float t=tmr.difference(t0, tmr.sample());
    s+=t;
    s2+=t*t;
    if(n>2 && sqrt((s2-s*s/n)/n/(n-1))<s/n*q)
      return s/n;
  }
}

template <class F>
float run_bench(F f, float q)
{
  return run_bench(f, q, clock_timer());
}

template <class ... Ts> struct type_list;
template <class A, class B > struct type_list_append;
template <class T, class ... Ts> struct type_list_append<type_list<Ts...>, T> {
	using type = type_list<Ts..., T>;
};
template < class B, template <class> class F > struct type_list_map;
template <template <class> class F, class ... Ts> struct type_list_map<type_list<Ts...>, F> {
	using type = type_list<F<Ts> ...>;
};
template <class ... Ts>
struct seq;
template <>
struct seq<>
{
	using type = type_list<>;
};

template <class ... Ts, class T>
struct seq<T, Ts...> {
	using type = typename type_list_append<typename seq<Ts...>::type, std::integral_constant<int, sizeof...(Ts)> >::type;
};

template <class A, class B> using map_to = B;

template <class ... Is, template <class...> class Seq, class F>
auto return_it(F f, Seq<Is...>* s) {
	return std::make_tuple(f(Is::value)...);
}

float sqr(float a) { return a*a; }

template<class Timer, class ... Fs>
std::tuple<map_to<Fs, float>...> tournament(Timer tmr, Fs &&... fs)
{
	using detail::prepare;
	std::function<void()> prepare_f[sizeof...(fs)] { [&] { prepare(fs); } ... };
	std::function<void()> t_funcs[sizeof...(fs)] { [&]{ fs(); } ... };
	float s [ sizeof...(fs) ] { 0 };
	float s2 [ sizeof...(fs) ] { 0 };
	float n [ sizeof...(fs) ] { 0 };

	std::ofstream o("benchmark-data.out");
	auto avg = [&](int i) {
							 return s[i]/n[i];
						 };
	auto variance = [&](int i) {
										return (s2[i]-s[i]*s[i]/n[i])/n[i]/(n[i]-1);
									};
	auto iteration = [&](int i) {
										 prepare_f[i]();
										 typename Timer::time_t t0 = tmr.sample();
										 t_funcs[i]();
										 float elapsed = tmr.difference(t0, tmr.sample());
										 o << i << "," << elapsed << "\n";
										 s[i] += elapsed;
										 s2[i] += elapsed*elapsed;
										 n[i]++;
									 };
	auto fight = [&] (int i, int j) {
								 prepare_f[i](); t_funcs[i]();
								 prepare_f[j](); t_funcs[j]();
								 for(int k = n[i]; k < 5; k++) iteration(i);
								 for(int k = n[j]; k < 5; k++) iteration(j);
								 while(!(sqr(avg(i)-avg(j)) >= 2*(variance(i) + variance(j)))) {
									 iteration(variance(i)/n[i] > variance(j)/n[j] ? i : j);
								 }
								 return avg(i) < avg(j) ? i : j;
							 };
	int candidate = 0;
	for(int i = 1; i < sizeof...(fs); i++) {
		candidate = fight(candidate, i);
	}

	return return_it(avg, (typename seq<Fs...>::type*)0);

}

// Local variables:
// mode: c++
// mode: doxymacs
// indent-tabs-mode: t
// End:
