#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <cuv_general.hpp>
#include <dev_dense_matrix.hpp>
#include <host_dense_matrix.hpp>
#include <convert.hpp>
#include <matrix_ops.hpp>
#include <matrix_ops/rprop.hpp>

using namespace cuv;

struct Fix{
	static const int n=256;
	static const int N=n*n;
	dev_dense_matrix<float> a,b,u,v,w;
	host_dense_matrix<float> s,t,r,x,z;
	Fix()
	:   a(1,n),b(1,n),u(n,n),v(n,n),w(n,n)
	,   s(1,n),t(1,n),r(n,n),x(n,n),z(n,n)
	{
	}
	~Fix(){
	}
};


BOOST_FIXTURE_TEST_SUITE( s, Fix )


BOOST_AUTO_TEST_CASE( vec_ops_unary1 )
{
	apply_scalar_functor(v, SF_EXP);
	apply_scalar_functor(v, SF_EXACT_EXP);
	apply_scalar_functor(x, SF_EXP);
	apply_scalar_functor(x, SF_EXACT_EXP);
}

BOOST_AUTO_TEST_CASE( vec_ops_binary1 )
{
	sequence(v.vec());
	sequence(w.vec());
	apply_scalar_functor(v,SF_ADD,1);
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(v.vec()[i], i + 1);
	}
	apply_binary_functor(v,w, BF_ADD);
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(v.vec()[i], i + i + 1);
	}
}

BOOST_AUTO_TEST_CASE( vec_ops_binary2 )
{
	apply_binary_functor(v,w, BF_AXPY, 2);
}

BOOST_AUTO_TEST_CASE( vec_ops_copy )
{
	// generate data
	sequence(w);

	// copy data from v to w
	copy(v,w);
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(v.vec()[i],w.vec()[i]);
	}
}

BOOST_AUTO_TEST_CASE( vec_ops_unary_add )
{
	sequence(v);
	apply_scalar_functor(v,SF_ADD,3.8f);
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(v.vec()[i], i+3.8f);
	}
}
BOOST_AUTO_TEST_CASE( vec_ops_axpby )
{
	// generate data
	sequence(w);
	sequence(v);
	apply_scalar_functor(v,SF_ADD, 1.f);
	BOOST_CHECK_EQUAL(w.vec()[0],0);
	BOOST_CHECK_EQUAL(v.vec()[0],1);

	// copy data from v to w
	apply_binary_functor(v,w,BF_AXPBY, 2.f,3.f);
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(v.vec()[i], 2*(i+1) + 3*i );
	}
}


BOOST_AUTO_TEST_CASE( vec_ops_0ary1 )
{
	// test sequence
	sequence(v.vec());
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(v.vec()[i], i);
	}

	// test fill
	fill(w.vec(),1);
	for(int i=0;i<N;i++){
		BOOST_CHECK_EQUAL(w.vec()[i], 1);
	}
}

BOOST_AUTO_TEST_CASE( vec_ops_norms )
{
	sequence(v.vec());
	float f1 = norm1(v), f1_ = 0;
	float f2 = norm2(v), f2_ = 0;
	for(int i=0;i<N;i++){
		f2_ += v.vec()[i]*v.vec()[i];
		f1_ += fabs(v.vec()[i]);
	}
	f2_ = sqrt(f2_);
	BOOST_CHECK_CLOSE(f1,f1_,0.01f);
	BOOST_CHECK_CLOSE(f2,f2_,0.01f);
}


BOOST_AUTO_TEST_CASE( mat_op_mm )
{
	sequence(v);     apply_scalar_functor(v, SF_MULT, 0.01f);
	sequence(w);     apply_scalar_functor(w, SF_MULT, 0.01f);
	sequence(x);     apply_scalar_functor(x, SF_MULT, 0.01f);
	sequence(z);     apply_scalar_functor(z, SF_MULT, 0.01f);
	prod(u,v,w,'n','t');
	prod(r,x,z,'n','t');

	host_dense_matrix<float> u2(u.h(), u.w());
	convert(u2,u);
	for(int i=0;i<256;i++){
		for(int j=0;j<256;j++){
			BOOST_CHECK_CLOSE( u2(i,j), r(i,j), 0.01 );
		}
	}
}

BOOST_AUTO_TEST_CASE( mat_op_mmdim1 )
{
	sequence(a);     apply_scalar_functor(a, SF_MULT, 0.01f);
	sequence(w);     apply_scalar_functor(w, SF_MULT, 0.01f);
	sequence(s);     apply_scalar_functor(s, SF_MULT, 0.01f);
	sequence(z);     apply_scalar_functor(z, SF_MULT, 0.01f);
	prod(b,a,w,'n','t');
	prod(t,s,z,'n','t');

	host_dense_matrix<float> b2(b.h(), b.w());
	convert(b2,b);

	for(int i=0;i<256;i++) {
		float val = 0.0f;
		for(int j=0;j<256;j++) {
			val += s(0,j) * z(i,j);
		}
		BOOST_CHECK_CLOSE( b2(0,i), val, 0.01 );
		BOOST_CHECK_CLOSE( t(0,i), val, 0.01 );
	}
}

BOOST_AUTO_TEST_CASE( mat_op_mat_plus_vec )
{
	sequence(v); sequence(w);
	sequence(x); sequence(z);
	dev_vector<float>   v_vec(n); sequence(v_vec);
	host_vector<float>  x_vec(n); sequence(x_vec);
	matrix_plus_col(v,v_vec);
	matrix_plus_col(x,x_vec);
	for(int i=0;i<n;i++){
		for(int j=0;j<n;j++){
			BOOST_CHECK_CLOSE(v(i,j), x(i,j), 0.01);
			BOOST_CHECK_CLOSE(v(i,j), w(i,j)+v_vec[i], 0.01);
			BOOST_CHECK_CLOSE(x(i,j), z(i,j)+x_vec[i], 0.01);
		}
	}

}

BOOST_AUTO_TEST_CASE( mat_op_mat_plus_vec_row_major )
{
	dev_dense_matrix<float,  row_major> V(v.h(),v.w()); sequence(V);
	host_dense_matrix<float, row_major> X(x.h(),x.w()); sequence(X);
	dev_vector<float>   v_vec(n); sequence(v_vec);
	host_vector<float>  x_vec(n); sequence(x_vec);
	matrix_plus_col(V,v_vec);
	matrix_plus_col(X,x_vec);
	for(int i=0;i<n;i++){
		for(int j=0;j<n;j++){
			BOOST_CHECK_CLOSE(v(i,j), x(i,j), 0.01);
			BOOST_CHECK_CLOSE(v(i,j), w(i,j)+v_vec[i], 0.01);
			BOOST_CHECK_CLOSE(x(i,j), z(i,j)+x_vec[i], 0.01);
		}
	}

}

BOOST_AUTO_TEST_CASE( mat_op_reduce_to_col )
{
	sequence(v);
	sequence(x);
	dev_vector<float>  v_col(n);
	host_vector<float> x_col(n);
	reduce_to_col(v_col,v);
	reduce_to_col(x_col,x);
	for(int i=0;i<n;i++){
		float v_correct = 0;
		for(int j=0;j<n;j++)
			v_correct += v(i,j);
		BOOST_CHECK_CLOSE(v_correct,v_col[i],0.01);
		BOOST_CHECK_CLOSE(v_col[i],x_col[i],0.01);
	}
}


BOOST_AUTO_TEST_SUITE_END()
