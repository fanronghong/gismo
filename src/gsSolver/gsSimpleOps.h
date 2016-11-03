/** @file gsSimpleOps.h

    @brief Colloction of some simple preconditioners.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): J. Sogn, C. Hofreither
*/
#pragma once

#include <gsCore/gsLinearAlgebra.h>
#include <gsSolver/gsLinearOperator.h>

namespace gismo
{
    
/// @brief Update \a x with a Richardson sweep
/// \ingroup Solver
GISMO_EXPORT void dampedRichardsonSweep(const gsSparseMatrix<real_t>& A, gsMatrix<real_t>& x, const gsMatrix<real_t>& f, real_t tau = (real_t)(1.));

/// @brief Update \a x with a Jacobi sweep
/// \ingroup Solver
GISMO_EXPORT void JacobiSweep(const gsSparseMatrix<real_t>& A, gsMatrix<real_t>& x, const gsMatrix<real_t>& f);

/// @brief Update \a x with a damped Jacobi sweep
/// \ingroup Solver
GISMO_EXPORT void dampedJacobiSweep(const gsSparseMatrix<real_t>& A, gsMatrix<real_t>& x, const gsMatrix<real_t>& f, real_t tau = (real_t)(0.5));

/// @brief Update \a x with a forward Gauss-Seidel sweep
/// \ingroup Solver
GISMO_EXPORT void gaussSeidelSweep(const gsSparseMatrix<real_t>& A, gsMatrix<real_t>& x, const gsMatrix<real_t>& f);

/// @brief Update \a x with a backward Gauss-Seidel sweep
/// \ingroup Solver
GISMO_EXPORT void reverseGaussSeidelSweep(const gsSparseMatrix<real_t>& A, gsMatrix<real_t>& x, const gsMatrix<real_t>& f);

/// @brief Preforms a block Gauss-Seidel on the degrees of freedom in DoFs.
/// \inrgoup Solver
GISMO_EXPORT void gaussSeidelSingleBlock(const gsSparseMatrix<real_t>& A, gsMatrix<real_t>& x, const gsMatrix<real_t>& f, gsVector<index_t>& DoFs);


/// @brief Richardson preconditioner
///
/// \ingroup Solver
template <typename MatrixType>
class gsRichardsonOp : public gsLinearOperator<typename MatrixType::Scalar>
{
public:
    typedef typename MatrixType::Scalar T;
    
    /// Shared pointer for gsRichardsonOp
    typedef typename memory::shared< gsRichardsonOp >::ptr Ptr;

    /// Unique pointer for gsRichardsonOp   
    typedef typename memory::unique< gsRichardsonOp >::ptr uPtr;    
    
    /// @brief Contructor with given matrix
    gsRichardsonOp(const MatrixType& _mat, T tau = 1.)
        : m_mat(_mat), m_numOfSweeps(1), m_tau(tau) {}
        
    static Ptr make(const MatrixType& _mat, T tau = 1.)
    { return memory::make_shared( new gsRichardsonOp(_mat,tau) ); }

    void apply(const gsMatrix<T> & input, gsMatrix<T> & x) const
    {
        GISMO_ASSERT( m_mat.rows() == input.rows() && m_mat.cols() == m_mat.rows()
                      && input.cols() == 1, "Dimensions do not match");

        // For the first sweep, we do not need to multiply with the matrix
        x = m_tau * input;

        gsMatrix<T> temp;
        for (index_t k = 1; k < m_numOfSweeps; ++k)
        {
            temp.noalias() = input - m_mat * x;
            x += m_tau * temp;
        }
    }

    index_t rows() const {return m_mat.rows();}
    index_t cols() const {return m_mat.cols();}

    /// Set number of sweeps.
    void setNumOfSweeps(index_t n)
    {
        GISMO_ASSERT ( n > 0, "Number of sweeps needs to be positive. ");
        m_numOfSweeps=n;
    }

    ///Returns the matrix
    const MatrixType & matrix() const { return m_mat; }

private:
    MatrixType m_mat; // expensive copy
    index_t m_numOfSweeps;
    T m_tau;
};

/// @brief Jacobi preconditioner
///
/// Requires a positive definite matrix.
///
/// \ingroup Solver
template <typename MatrixType>
class gsJacobiOp : public gsLinearOperator<typename MatrixType::Scalar>
{
public:
    typedef typename MatrixType::Scalar T;
    
    /// Shared pointer for gsJacobiOp
    typedef typename memory::shared< gsJacobiOp >::ptr Ptr;

    /// Unique pointer for gsJacobiOp   
    typedef typename memory::unique< gsJacobiOp >::ptr uPtr;    

    /// @brief Contructor with given matrix
    gsJacobiOp(const MatrixType& _mat, T tau = 1.)
        : m_mat(_mat), m_numOfSweeps(1), m_tau(tau) {}
        
    static Ptr make(const MatrixType& _mat, T tau = 1.)
    { return memory::make_shared( new gsJacobiOp(_mat,tau) ); }

    void apply(const gsMatrix<T> & input, gsMatrix<T> & x) const
    {
        GISMO_ASSERT( m_mat.rows() == input.rows() && m_mat.cols() == m_mat.rows() && input.cols() == 1, "Dimensions do not match.");

        // For the first sweep, we do not need to multiply with the matrix
        x.array() = m_tau * input.array() / m_mat.diagonal().array();
        
        for (index_t k = 1; k < m_numOfSweeps; ++k)
            x.array() += m_tau * ( input - m_mat * x ).array() / m_mat.diagonal().array();
    }

    index_t rows() const {return m_mat.rows();}
    index_t cols() const {return m_mat.cols();}

    /// Set number of sweeps.
    void setNumOfSweeps(index_t n) {
        GISMO_ASSERT ( n > 0, "Number of sweeps needs to be positive. ");
        m_numOfSweeps=n;
    }

    ///Returns the matrix
    const MatrixType & matrix() const { return m_mat; }

private:
    MatrixType m_mat; // expensive copy
    index_t m_numOfSweeps;
    T m_tau;
};

template <class Derived>
typename gsJacobiOp<Derived>::Ptr makeJacobiOp(const Eigen::EigenBase<Derived>& mat,
                                               typename Derived::Scalar tau = 1)
{ return gsJacobiOp<Derived>::make(mat.derived(), tau); }


/// @brief Gauss-Seidel preconditioner
///
/// Requires a positive definite matrix.
///
/// \ingroup Solver
template <typename MatrixType>
class gsGaussSeidelOp : public gsLinearOperator<typename MatrixType::Scalar>
{
public:
    typedef typename MatrixType::Scalar T;

    /// Shared pointer for gsGaussSeidelOp
    typedef typename memory::shared< gsGaussSeidelOp >::ptr Ptr;

    /// Unique pointer for gsGaussSeidelOp   
    typedef typename memory::unique< gsGaussSeidelOp >::ptr uPtr;   
    
    /// @brief Contructor with given matrix
    gsGaussSeidelOp(const MatrixType& _mat)
        : m_mat(_mat), m_numOfSweeps(1) {}
        
    static Ptr make(const MatrixType& _mat)
    { return memory::make_shared( new gsGaussSeidelOp(_mat) ); }

    void apply(const gsMatrix<T> & input, gsMatrix<T> & x) const
    {
        x.setZero(rows(), input.cols());

        for (index_t k = 0; k < m_numOfSweeps; ++k)
        {
            gaussSeidelSweep(m_mat,x,input);
        }
    }

    index_t rows() const {return m_mat.rows();}
    index_t cols() const {return m_mat.cols();}

    /// Set number of sweeps of to symmetric Gauss-Seidel perform (default is 1).
    void setNumOfSweeps(index_t n) {
        GISMO_ASSERT ( n > 0, "Number of sweeps needs to be positive. ");
        m_numOfSweeps=n;
    }

    ///Returns the matrix
    const MatrixType & matrix() const { return m_mat; }
    
private:
    MatrixType m_mat; // expensive copy
    index_t m_numOfSweeps;
};


/// @brief Symmetric Gauss-Seidel preconditioner
///
/// Requires a positive definite matrix. Does first
/// one forward Gauss-Seidel sweep then one backward
/// Gauss-Seidel sweep.
///
/// \ingroup Solver
template <typename MatrixType>
class gsSymmetricGaussSeidelOp : public gsLinearOperator<typename MatrixType::Scalar>
{
    typedef typename memory::shared<MatrixType>::ptr MatrixPtr;
    typedef typename MatrixType::Nested     NestedMatrix;

public:
    typedef typename MatrixType::Scalar T;
    
    /// Shared pointer for gsSymmetricGaussSeidelOp
    typedef typename memory::shared< gsSymmetricGaussSeidelOp >::ptr Ptr;

    /// Unique pointer for gsSymmetricGaussSeidelOp   
    typedef typename memory::unique< gsSymmetricGaussSeidelOp >::ptr uPtr; 
    
    /// @brief Contructor with given matrix
    explicit gsSymmetricGaussSeidelOp(const MatrixType& _mat)
    : m_mat(), m_expr(_mat.derived()), m_numOfSweeps(1) {}

    /// @brief Contructor with shared pointer to matrix
    explicit gsSymmetricGaussSeidelOp(const MatrixPtr& _mat)
    : m_mat(_mat), m_expr(m_mat->derived()), m_numOfSweeps(1) {}
    
    static Ptr make(const MatrixType& _mat) 
    { return memory::make_shared( new gsSymmetricGaussSeidelOp(_mat) ); }

    static Ptr make(const MatrixPtr& _mat) 
    { return memory::make_shared( new gsSymmetricGaussSeidelOp(_mat) ); }

    void apply(const gsMatrix<T> & input, gsMatrix<T> & x) const
    {
        x.setZero(rows(), input.cols());

        for (index_t k = 0; k < m_numOfSweeps; ++k)
        {
            gaussSeidelSweep(m_expr,x,input);
            //x.array() *= m_expr.diagonal().array();
            reverseGaussSeidelSweep(m_expr,x,input);
        }
    }

    index_t rows() const {return m_expr.rows();}

    index_t cols() const {return m_expr.cols();}

    /// Set number of sweeps of to symmetric Gauss-Seidel perform
    /// (default is 1).
    void setNumOfSweeps(index_t n)    { m_numOfSweeps= n; }

    ///Returns the matrix
    NestedMatrix matrix() const { return m_expr; }

private:
    const MatrixPtr m_mat;  ///< Shared pointer to matrix (if needed)
    NestedMatrix    m_expr; ///< Nested Eigen expression

    index_t m_numOfSweeps;
};


} // namespace gismo
