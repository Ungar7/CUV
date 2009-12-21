#define BOOST_TEST_MODULE example
#include <cstdio>
#include <boost/test/included/unit_test.hpp>

#include <cuv_general.hpp>
#include <dev_dense_matrix.hpp>
#include <host_dense_matrix.hpp>
#include <vector_ops.hpp>
#include <timing.hpp>
#include <random.hpp>
#include <vector_ops/rprop.hpp>

using namespace cuv;

#define MEASURE_TIME(MSG, OPERATION, ITERS)     \
	float MSG;                                  \
	if(1){                                      \
		Timing tim;                             \
		for(int i=0;i<ITERS;i++){               \
			OPERATION ;                         \
		}                                       \
		tim.update(ITERS);                      \
		printf("%s [%s] took %4.4f us/pass\n", #MSG, #OPERATION, 1000000.0f*tim.perf()); \
		MSG = 1000000.0f*tim.perf();            \
	}

struct MyConfig {
	static const int dev = 2;
	MyConfig()   { 
		printf("Testing on device=%d\n",dev);
		initCUDA(dev); 
		initialize_mersenne_twister_seeds();
	}
	~MyConfig()  { exitCUDA();  }
};


BOOST_GLOBAL_FIXTURE( MyConfig );


struct Fix{
	static const int n = 784*2048; 
	dev_vector<float>  v,w;
	host_vector<float> x,z;
	Fix()
	:   v(n),w(n)
	,   x(n),z(n)
	{
		//MEASURE_TIME("warmup", apply_scalar_functor(v, SF_EXP), 100);
	}
	~Fix(){
	}
};


BOOST_FIXTURE_TEST_SUITE( s, Fix )


BOOST_AUTO_TEST_CASE( vec_rnd )
{
	MEASURE_TIME(rnd_normal, fill_rnd_normal(v), 100);

	MEASURE_TIME(rnd_uniform,      fill_rnd_uniform(v), 100);
	MEASURE_TIME(rnd_uniform_host, float f=0;for(int k=0;k<n;k++) f+=((float)rand()/RAND_MAX); , 100);
	printf("Speedup: %3.4f\n", rnd_uniform_host/rnd_uniform);
}

BOOST_AUTO_TEST_CASE( vec_ops_exp )
{
	sequence(v);
	sequence(x);
	MEASURE_TIME(dev , apply_scalar_functor(v, SF_EXP), 1000);
	MEASURE_TIME(host, apply_scalar_functor(x, SF_EXP), 1000);
	printf("Speedup: %3.4f\n", host/dev);
}

BOOST_AUTO_TEST_CASE( vec_ops_unary2 )
{
	sequence(v);
	sequence(x);
	MEASURE_TIME(mult_dev, apply_scalar_functor(v, SF_MULT,0.01f), 1000);
	MEASURE_TIME(mult_host, apply_scalar_functor(x, SF_MULT,0.01f), 1000);
	printf("Speedup: %3.4f\n", mult_host/mult_dev);
	MEASURE_TIME(add_dev,  apply_scalar_functor(v, SF_ADD,0.01f), 1000);
	MEASURE_TIME(add_host,  apply_scalar_functor(x, SF_ADD,0.01f), 1000);
	printf("Speedup: %3.4f\n", add_host/add_dev);
}

BOOST_AUTO_TEST_CASE( vec_xpby )
{
	sequence(v);
	sequence(w);
	sequence(x);
	sequence(z);
	MEASURE_TIME(dev, apply_binary_functor(v,w, BF_XPBY,1.8f), 1000);
	MEASURE_TIME(host, apply_binary_functor(x,z, BF_XPBY,1.8f), 1000);
	printf("Speedup: %3.4f\n", host/dev);
}

BOOST_AUTO_TEST_CASE( vec_add )
{
	sequence(v);
	sequence(w);
	sequence(x);
	sequence(z);
	MEASURE_TIME(dev, apply_binary_functor(v,w, BF_ADD), 1000);
	MEASURE_TIME(host, apply_binary_functor(x,z, BF_ADD), 1000);
	printf("Speedup: %3.4f\n", host/dev);
}


BOOST_AUTO_TEST_CASE( vec_rprop )
{
	dev_vector<signed char> dW_old(n);
	dev_vector<float>       dW(n);
	dev_vector<float>       W(n);
	dev_vector<float>       rate(n);
	host_vector<signed char> h_dW_old(n);
	host_vector<float>       h_W(n);
	host_vector<float>       h_dW(n);
	host_vector<float>       h_rate(n);
	sequence(dW);           apply_scalar_functor(dW, SF_ADD, -10.f);
	sequence(dW_old);
	fill(rate, 1.f);
	sequence(h_dW);         apply_scalar_functor(dW, SF_ADD, -10.f);
	sequence(h_dW_old);
	fill(h_rate, 1.f);

	MEASURE_TIME(dev,  cuv::rprop(W,dW,dW_old,rate), 10);
	MEASURE_TIME(host, cuv::rprop(h_W,h_dW,h_dW_old,h_rate), 10);
	printf("Speedup: %3.4f\n", host/dev);
}


BOOST_AUTO_TEST_CASE( vec_lswd )
{
	dev_vector<float>       dW(n);
	dev_vector<float>       W(n);
	host_vector<float>       h_W(n);
	host_vector<float>       h_dW(n);

	MEASURE_TIME(dev,  cuv::learn_step_weight_decay(W,dW,1.f,0.05f), 10);
	MEASURE_TIME(host, cuv::learn_step_weight_decay(h_W,h_dW,1.f,0.05f), 10);
	printf("Speedup: %3.4f\n", host/dev);
}


BOOST_AUTO_TEST_SUITE_END()