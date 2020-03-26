/** @file gsGluingData.h

    @brief Compute the gluing data for one interface.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): P. Weinmueller
*/

#pragma once

# include <gsG1Basis/gsGlobalGDAssembler.h>
# include <gsG1Basis/gsLocalGDAssembler.h>

# include <gsG1Basis/gsG1OptionList.h>

namespace gismo
{

template<class T>
class gsApproxGluingData
{
public:
    gsApproxGluingData()
    { }

    gsApproxGluingData(gsMultiPatch<T> const & mp,
                 gsMultiBasis<T> const & mb,
                 index_t uv,
                 bool isBoundary,
                 gsG1OptionList & g1OptionList)
        : m_mp(mp), m_mb(mb), m_uv(uv), m_isBoundary(isBoundary)
    {
        m_gamma = 1.0;

        p_tilde = g1OptionList.getInt("p_tilde");
        r_tilde = g1OptionList.getInt("r_tilde");

        m_r = g1OptionList.getInt("regularity");
    }

    // Computed the gluing data globally
    void setGlobalGluingData();

    // Computed the gluing data locally
    void setLocalGluingData(gsBSplineBasis<> & basis_plus, gsBSplineBasis<> & basis_minus);

    const gsBSpline<T> get_alpha_tilde() const {return alpha_tilde; }
    const gsBSpline<T> get_beta_tilde() const {return beta_tilde; }

    const gsBSpline<T> get_local_alpha_tilde(index_t i) const {return alpha_minus_tilde[i]; }
    const gsBSpline<T> get_local_beta_tilde(index_t i) const {return beta_plus_tilde[i]; }

protected:
    // The geometry for a single interface in the right parametrizations
    gsMultiPatch<T> m_mp;
    gsMultiBasis<T> m_mb;
    index_t m_uv;
    bool m_isBoundary;

    real_t m_gamma;

    // Spline space for the gluing data (p_tilde,r_tilde,k)
    index_t p_tilde, r_tilde;

    // Regularity of the geometry
    index_t m_r;

protected:
    // Global Gluing data
    gsBSpline<T> alpha_tilde;
    gsBSpline<T> beta_tilde;

    // Local Gluing data
    std::vector<gsBSpline<T>> alpha_minus_tilde, beta_plus_tilde;

}; // class gsGluingData


template<class T>
void gsApproxGluingData<T>::setGlobalGluingData()
{
    // ======== Space for gluing data : S^(p_tilde, r_tilde) _k ========
    gsKnotVector<T> kv(0,1,0,p_tilde+1,p_tilde-r_tilde); // first,last,interior,mult_ends,mult_interior
    gsBSplineBasis<T> bsp_gD(kv);

    gsBSplineBasis<> temp_basis_first = dynamic_cast<gsBSplineBasis<> &>(m_mb.basis(0).component(m_uv)); // u
    //gsBSplineBasis<> temp_basis_second = dynamic_cast<gsBSplineBasis<> &>(m_mb.basis(1).component(1)); // v
/*
    if (temp_basis_first.numElements() >= temp_basis_second.numElements())
    {
        index_t degree = temp_basis_second.maxDegree();
        for (size_t i = degree+1; i < temp_basis_second.knots().size() - (degree+1); i = i+(degree-m_r))
            bsp_gD.insertKnot(temp_basis_second.knot(i),p_tilde-r_tilde);
    }
    else
    {
        index_t degree = temp_basis_first.maxDegree();
        for (size_t i = degree+1; i < temp_basis_first.knots().size() - (degree+1); i = i+(degree-m_r))
            bsp_gD.insertKnot(temp_basis_first.knot(i),p_tilde-r_tilde);

    }
*/

    index_t degree = temp_basis_first.maxDegree();
    for (size_t i = degree+1; i < temp_basis_first.knots().size() - (degree+1); i = i+(degree-m_r))
        bsp_gD.insertKnot(temp_basis_first.knot(i),p_tilde-r_tilde);

    gsGlobalGDAssembler<T> globalGdAssembler(bsp_gD, m_uv, m_mp, m_gamma, m_isBoundary);
    globalGdAssembler.assemble();

    gsSparseSolver<real_t>::CGDiagonal solver;
    gsVector<> sol_a, sol_b;

    // alpha^S
    solver.compute(globalGdAssembler.matrix_alpha());
    sol_a = solver.solve(globalGdAssembler.rhs_alpha());

    gsGeometry<>::uPtr tilde_temp;
    tilde_temp = bsp_gD.makeGeometry(sol_a);
    gsBSpline<T> alpha_tilde_2 = dynamic_cast<gsBSpline<T> &> (*tilde_temp);
    alpha_tilde = alpha_tilde_2;

    // beta^S
    solver.compute(globalGdAssembler.matrix_beta());
    sol_b = solver.solve(globalGdAssembler.rhs_beta());

    tilde_temp = bsp_gD.makeGeometry(sol_b);
    gsBSpline<T> beta_tilde_2 = dynamic_cast<gsBSpline<T> &> (*tilde_temp);
    beta_tilde = beta_tilde_2;

} // setGlobalGluingData

template<class T>
void gsApproxGluingData<T>::setLocalGluingData(gsBSplineBasis<> & basis_plus, gsBSplineBasis<> & basis_minus)
{
    index_t n_plus = basis_plus.size();
    index_t n_minus = basis_minus.size();

    // Setting the space for each alpha_tilde, beta_tilde
    alpha_minus_tilde.resize(n_minus);
    beta_plus_tilde.resize(n_plus);

    // ======== Space for gluing data : S^(p_tilde, r_tilde) _k ========
    gsKnotVector<T> kv(0,1,0,p_tilde+1,p_tilde-r_tilde); // first,last,interior,mult_ends,mult_interior
    gsBSplineBasis<T> bsp_gD(kv);

    gsBSplineBasis<> temp_basis_first = dynamic_cast<gsBSplineBasis<> &>(m_mb.basis(0).component(m_uv)); // u

    index_t degree = temp_basis_first.maxDegree();
    for (size_t i = degree+1; i < temp_basis_first.knots().size() - (degree+1); i = i+(degree-m_r))
        bsp_gD.insertKnot(temp_basis_first.knot(i),p_tilde-r_tilde);

    // Compute alpha_minus
    for (index_t i = 0; i < n_minus; i++)
    {
        gsMatrix<T> ab = basis_minus.support(i);

        gsKnotVector<T> kv(ab.at(0), ab.at(1),0, p_tilde+1);

        index_t degree = temp_basis_first.maxDegree();
        for (size_t i = degree+1; i < temp_basis_first.knots().size() - (degree+1); i = i+(degree-m_r))
            if ((temp_basis_first.knot(i) > ab.at(0)) && (temp_basis_first.knot(i) < ab.at(1)))
                kv.insert(temp_basis_first.knot(i), p_tilde - r_tilde);
        /*
        real_t span = bsp_gD.getMaxCellLength();
        real_t temp_knot = ab.at(0) + span;
        while (temp_knot < ab.at(1))
        {
            kv.insert(temp_knot,p_tilde-r_tilde);
            temp_knot += span;
        }
         */
        gsBSplineBasis<T> bsp_geo(kv);

        // The first basis (bsp_geo) is for the gd, the second for the integral
        gsLocalGDAssembler<T> localGdAssembler(bsp_geo, bsp_geo, m_uv, m_mp, m_gamma, m_isBoundary, "alpha");
        localGdAssembler.assemble();

        gsSparseSolver<real_t>::CGDiagonal solver;
        gsVector<> sol;

        // alpha^S
        solver.compute(localGdAssembler.matrix());
        sol = solver.solve(localGdAssembler.rhs());

        gsGeometry<>::uPtr tilde_temp;
        tilde_temp = bsp_geo.makeGeometry(sol);
        gsBSpline<T> a_t = dynamic_cast<gsBSpline<T> &> (*tilde_temp);
        alpha_minus_tilde.at(i) = a_t;
    }
    for (index_t i = 0; i < n_plus; i++)
    {
        gsMatrix<T> ab = basis_plus.support(i);

        gsKnotVector<T> kv(ab.at(0), ab.at(1),0, p_tilde+1);

        index_t degree = temp_basis_first.maxDegree();
        for (size_t i = degree+1; i < temp_basis_first.knots().size() - (degree+1); i = i+(degree-m_r))
            if ((temp_basis_first.knot(i) > ab.at(0)) && (temp_basis_first.knot(i) < ab.at(1)))
                kv.insert(temp_basis_first.knot(i), p_tilde - r_tilde);
        /*
        real_t span = bsp_gD.getMaxCellLength();
        real_t temp_knot = ab.at(0) + span;
        while (temp_knot < ab.at(1))
        {
            kv.insert(temp_knot,p_tilde-r_tilde);
            temp_knot += span;
        }
         */
        gsBSplineBasis<T> bsp_geo(kv);

        // The first basis (bsp_geo) is for the gd, the second for the integral
        gsLocalGDAssembler<T> localGdAssembler(bsp_geo, bsp_geo, m_uv, m_mp, m_gamma, m_isBoundary, "beta");
        localGdAssembler.assemble();

        gsSparseSolver<real_t>::CGDiagonal solver;
        gsVector<> sol;

        // alpha^S
        solver.compute(localGdAssembler.matrix());
        sol = solver.solve(localGdAssembler.rhs());

        gsGeometry<>::uPtr tilde_temp;
        tilde_temp = bsp_geo.makeGeometry(sol);
        gsBSpline<T> b_t = dynamic_cast<gsBSpline<T> &> (*tilde_temp);
        beta_plus_tilde.at(i) = b_t;
    }
} // setLocalGluingData


} // namespace gismo

