// This file is part of Hermes2D.
//
// Hermes2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D.  If not, see <http://www.gnu.org/licenses/>.
/*
#ifndef KELLY_TYPE_ADAPT_H
#define KELLY_TYPE_ADAPT_H

#include "adapt.h"
#include "../neighbor_search.h"
#include "../discrete_problem.h"

namespace Hermes
{
namespace Hermes2D
{
/// Functor representing the interface estimator scaling function.
class InterfaceEstimatorScalingFunction
{
private:
virtual double value(double e_diam, const std::string& e_marker) const = 0;
template<typename Scalar> friend class KellyTypeAdapt;
};

/// Pre-defined function used for scaling interface error estimates (see the KellyTypeAdapt constructor).
class ScaleByElementDiameter : public InterfaceEstimatorScalingFunction
{
private:
virtual double value(double e_diam, const std::string& e_marker) const {
return e_diam;
}
template<typename Scalar> friend class KellyTypeAdapt;
};

/// \class KellyTypeAdapt
/// \ingroup g_adapt
/// \brief A framework for explicit aposteriori error estimators.
///
/// Explicit error estimators estimate the error of approximate solution on an element by evaluating
/// element residuals and jumps of the solution across element edges ([2]). A typical example is
/// the Kelly error estimator ([1]) where a sum of the L2 norms of element residual and jumps of
/// solution gradients across the element boundaries defines the element error.
///
/// References:
///  [1] Kelly D. W., Gago O. C., Zienkiewicz O. C., Babuska I.:
///&nbsp;    A posteriori error analysis and adaptive processes in the finite element method: Part I—error analysis.
///&nbsp;    Int. J. Numer. Methods Engng. 1983;19:1593–619.
///  [2] Gratsch T., Bathe K. J.:
///&nbsp;    A posteriori error estimation techniques in practical finite element analysis.
///&nbsp;    Computers and Structures 83 (2005) 235–265.
///  [3] Zienkiewicz O. C., Taylor R. L., Zhu J. Z.:
///&nbsp;    The finite element method: its basis and fundamentals (Section 13.7.1).
///&nbsp;    6th ed. (2005), Elsevier.
///
template<typename Scalar>
class HERMES_API KellyTypeAdapt : public Adapt<Scalar>
{
public:
/// Class representing the weak form of an error estimator.
///
/// A user must derive his own representation of the estimator from this class (an example is provided by the class
/// \c BasicKellyAdapt below). The three attributes have the following meaning:
///
///&nbsp;- i     ... with a multi-component solution, this defines for which component this estimate applies,
///&nbsp;- area  ... defines in which geometric parts of the domain should the estimate be used - e.g. by defining
///&nbsp;            area = H2D_DG_INNER_EDGE, errors at element interfaces will be tracked by the estimator,
///&nbsp;- ext   ... vector with external functions possibly used within the estimator (e.g. previous time-level
///&nbsp;            solutions appearing in the residual) - currently not used.
///
/// Every estimator form must also implement the two methods \c ord and \c value, which are used for determining
/// the integration order and for the actual evaluation of the form, respectively. During the evaluation, their
/// parameters will be interpreted as follows:
///
///&nbsp;- int n,                 ... number of integration points in the currently processed element
///&nbsp;- double *wt,            ... corresponding integration weights
///&nbsp;- Func\<Scalar\> *u[],   ... all solution components
///&nbsp;- Func\<double\> *u,     ... currently processed solution component
///&nbsp;- Geom\<double\> *e,     ... geometric data of the currently processed element
///
class HERMES_API ErrorEstimatorForm : public Form<Scalar>
{
public:
/// Component.
int i;
/// Geometric region where this estimator is applied.
std::string area;
/// Additional functions required by the estimator.
std::vector<MeshFunctionSharedPtr<Scalar> > ext;
/// Set this error form to be an interface one.
void setAsInterface();
/// Constructor.
ErrorEstimatorForm(int i, std::string area = HERMES_ANY,
std::vector<MeshFunctionSharedPtr<Scalar> > ext = std::vector<MeshFunctionSharedPtr<Scalar> >())
: i(i), area(area), ext(ext) {}

/// Value calculation.
virtual Scalar value(int n, double *wt, Func<Scalar> *u_ext[],
DiscontinuousFunc<Scalar> *u, Geom<double> *e,
Func<Scalar> **ext) const
{
throw Exceptions::MethodNotOverridenException("KellyTypeAdapt::ErrorEstimatorForm::value()");
return 0.0;
}

/// Integration order.
virtual Hermes::Ord ord(int n, double *wt, Func<Hermes::Ord> *u_ext[],
DiscontinuousFunc<Hermes::Ord> *u, Geom<Hermes::Ord> *e,
Func<Ord> **ext) const
{
throw Exceptions::MethodNotOverridenException("KellyTypeAdapt::ErrorEstimatorForm::ord().");
return Hermes::Ord();
}

/// FIXME - temporary
KellyTypeAdapt *adapt;
};

protected:
// Only needed for gaining access to NeighborSearch methods.
DiscreteProblem<Scalar> dp;

///
/// Functions used for evaluating the actual error estimator forms for an active element or edge segment.
///
double eval_volumetric_estimator(typename KellyTypeAdapt::ErrorEstimatorForm* err_est_form,
RefMap* rm);
double eval_boundary_estimator(typename KellyTypeAdapt::ErrorEstimatorForm* err_est_form,
RefMap* rm,
SurfPos* surf_pos);
double eval_interface_estimator(typename KellyTypeAdapt::ErrorEstimatorForm* err_est_form,
RefMap *rm,
SurfPos* surf_pos,
LightArray<NeighborSearch<Scalar>*>& neighbor_searches,
int neighbor_index);
double eval_solution_norm(typename Adapt<Scalar>::MatrixFormVolError* form,
RefMap* rm,
MeshFunctionSharedPtr<Scalar> sln);

///
/// Linear forms used to calculate the error estimator value for each component.
///
std::vector<typename KellyTypeAdapt::ErrorEstimatorForm *> error_estimators_vol;
std::vector<typename KellyTypeAdapt::ErrorEstimatorForm *> error_estimators_surf;

Mesh::ElementMarkersConversion element_markers_conversion;
Mesh::BoundaryMarkersConversion boundary_markers_conversion;

/// Scaling of the interface error estimates. May be specified by the user during construction.
///
std::vector<const InterfaceEstimatorScalingFunction*> interface_scaling_fns;
/// Specifies whether the interface error estimators for each
bool use_aposteriori_interface_scaling;
///< component will be multiplied by \c interface_scaling_fns
///< after being evaluated.

///
/// Constant scaling. Reserved for the derived classes, not to be used by the user explicitly.
///
/// Constant scaling of the boundary error estimates.
double interface_scaling_const;
/// Constant scaling of the volumetric error estimates (like the residual norm).
double volumetric_scaling_const;
/// Constant scaling of the boundary error estimates.
double boundary_scaling_const;

/// Specifies whether the interface error estimator will be evaluated from each side of each interface
/// (when <c>ignore_visited_segments == false</c> ), or only once for each interface
/// (<c>ignore_visited_segments == true</c>).
bool ignore_visited_segments;

/// Calculates error estimates for each solution component, the total error estimate, and possibly also
/// their normalizations. If called with a pair of solutions, the version from Adapt is used (this is e.g.
/// done when comparing approximate solution to the exact one - in this case, we do not want to compute
/// the Kelly estimator value, but rather the ordinary difference between the solutions).
virtual double calc_err_internal(std::vector<MeshFunctionSharedPtr<Scalar> > slns,
std::vector<double>* component_errors,
unsigned int error_flags);

public:

/// Constructor.
///
/// \param[in]  spaces_   Approximation space of each solution component.
/// \param[in]  ignore_visited_segments_ If true, error estimator for each inner edge will be evaluated only
///&nbsp;                                   once. It will be added to the total error estimate for both the active
///&nbsp;                                   element and its neighbors across that edge, after possibly being scaled by
///&nbsp;                                   \c interface_scaling_fns_ for the current component (with the diameter of
///&nbsp;                                   the appropriate element). This saves duplicate evaluations with same
///&nbsp;                                   results when the estimator is given e.g. by the jumps of the solution.
///
///&nbsp;                                   If false, error estimator for each surface of each element will be
///&nbsp;                                   evaluated, regardless of whether the neighbor side of the interface
///&nbsp;                                   has already been processed.
///
///&nbsp;                                   Note that if \c interface_scaling_fns_ is empty (or unspecified) then the
///&nbsp;                                   default scaling by element diameter will be always performed unless it is
///&nbsp;                                   switched off by a call to \c disable_aposteriori_interface_scaling.
/// \param[in]  interface_scaling_fns_  Specifies functions used for scaling the interface error estimator for
///&nbsp;                                  each component. The scale is defined as a real function of the element
///&nbsp;                                  diameter (and possibly equation coefficients associated to the element)
///&nbsp;                                  and multiplies the result of the interface estimators. It may thus be already
///&nbsp;                                  present in the interface estimator forms themselves, in which case call
///&nbsp;                                  \c disable_aposteriori_interface_scaling. In this case, it may also be required
///&nbsp;                                  that \c ignore_visited_segments be false in order to always ensure that the
///&nbsp;                                  diameter belongs to the element whose error is being calculated.
/// \param[in]  norms_    Norms used for making relative error estimates.
///&nbsp;                    If not specified, they are defined according to the spaces.
///
///
KellyTypeAdapt(std::vector<SpaceSharedPtr<Scalar> > spaces,
bool ignore_visited_segments = true,
std::vector<const InterfaceEstimatorScalingFunction*>
interface_scaling_fns_ = std::vector<const InterfaceEstimatorScalingFunction*>(),
std::vector<NormType> norms_ = std::vector<NormType>());

KellyTypeAdapt(SpaceSharedPtr<Scalar> space,
bool ignore_visited_segments = true,
const InterfaceEstimatorScalingFunction* interface_scaling_fn_ = nullptr,
NormType norm_ = HERMES_UNSET_NORM);

/// Destructor.
virtual ~KellyTypeAdapt()
{
for (unsigned int i = 0; i < error_estimators_surf.size(); i++)
delete error_estimators_surf[i];
error_estimators_surf.clear();

for (unsigned int i = 0; i < error_estimators_vol.size(); i++)
delete error_estimators_vol[i];
error_estimators_vol.clear();

for (unsigned int i = 0; i < interface_scaling_fns.size(); i++)
delete interface_scaling_fns[i];
interface_scaling_fns.clear();
}

Mesh::ElementMarkersConversion* get_element_markers_conversion()
{
return &this->element_markers_conversion;
}
Mesh::BoundaryMarkersConversion* get_boundary_markers_conversion()
{
return &this->boundary_markers_conversion;
}

/// Append volumetric error estimator form.
///
/// For example, element residual norms may be represented by such a form.
///
/// \param[in]  form ... object representing the form. A class derived from \c KellyTypeAdapt::ErrorEstimatorForm
///&nbsp;                   defines its datatype.
///
void add_error_estimator_vol(ErrorEstimatorForm* form);

/// Append boundary or interface error estimator form.
///
/// Interface form is defined by <c> form::area == H2D_DG_INNER_EDGE </c>. The effective types for \c u_ext, \c u
/// and \c e (three of the obligatory parameters of form::value() and form::ord()) will then be, respectively
/// \c DiscontinuousFunc*[], \c DiscontinuousFunc* and \c InterfaceGeom*.
///
void add_error_estimator_surf(ErrorEstimatorForm* form);

///
/// The following two methods calculate the error of the given \c sln, using \code calc_err_internal \endcode.
///

double calc_err_est(MeshFunctionSharedPtr<Scalar>sln,
unsigned int error_flags = HERMES_TOTAL_ERROR_REL | HERMES_ELEMENT_ERROR_REL)
{
if(this->num != 1)
throw Exceptions::Exception("Wrong number of solutions.");
std::vector<MeshFunctionSharedPtr<Scalar> > slns;
slns.push_back(sln);
return calc_err_est(slns, nullptr, error_flags);
}

double calc_err_est(std::vector<MeshFunctionSharedPtr<Scalar> > slns,
std::vector<double>* component_errors = nullptr,
unsigned int error_flags = HERMES_TOTAL_ERROR_REL | HERMES_ELEMENT_ERROR_REL)
{
return calc_err_internal(slns, component_errors, error_flags);
}

/// Refines the elements selected by the \code RefinementSelectors::HOnlySelector \endcode according
/// to the errors calculated by \code calc_err_est \endcode.
///
bool adapt(double thr, int strat = 0, int regularize = -1, double to_be_processed = 0.0);

void disable_aposteriori_interface_scaling() { use_aposteriori_interface_scaling = false; }

void set_volumetric_scaling_const(double C) { volumetric_scaling_const = C; }
void set_boundary_scaling_const(double C) { boundary_scaling_const = C; }
};

/// \class BasicKellyAdapt
/// \ingroup g_adapt
/// \brief Simple Kelly-estimator based adaptivity for elliptic problems.
///
/// Original error estimator that Kelly et. al. ([1]) derived for the Laplace equation with constant
/// coefficient, approximated on a quadrilateral mesh. The error of each element is estimated by the
/// L2 norm of jumps of gradients across element faces (the contribution of the residual norm is
/// relatively insignificant and is neglected, see[3]). Note that the estimator has been successfully
/// used also for other problems than that for which it had been originally derived.
///
///\todo Add handling of boundary conditions.
///&nbsp;    Currently, the forms for the Neumann and Newton boundary conditions must be specified by
///&nbsp;    the user, see the example \c poisson-kelly-adapt.
///
template<typename Scalar>
class HERMES_API BasicKellyAdapt : public KellyTypeAdapt<Scalar>
{
public:
class HERMES_API ErrorEstimatorFormKelly : public KellyTypeAdapt<Scalar>::ErrorEstimatorForm
{
public:
/// Constructor.
ErrorEstimatorFormKelly(int i = 0, double const_by_laplacian = 1.0);

virtual Scalar value(int n, double *wt, Func<Scalar> *u_ext[],
DiscontinuousFunc<Scalar> *u, Geom<double> *e,
Func<Scalar> **ext) const
{
Scalar result = 0.;
if(u->fn_central != nullptr)
for (int i = 0; i < n; i++)
result += wt[i] * Hermes::sqr( const_by_laplacian * ( e->nx[i] * u->dx[i] + e->ny[i] * u->dy[i]));
else
for (int i = 0; i < n; i++)
result += wt[i] * Hermes::sqr( const_by_laplacian * ( e->nx[i] * u->dx_neighbor[i] + e->ny[i] * u->dy_neighbor[i]));

return result;
}

virtual Hermes::Ord ord(int n, double *wt, Func<Hermes::Ord> *u_ext[],
DiscontinuousFunc<Hermes::Ord> *u, Geom<Hermes::Ord> *e,
Func<Ord> **ext) const
{
if(u->fn_central != nullptr)
return Hermes::sqr(u->dx[0] + u->dy[0]);
else
return Hermes::sqr(u->dx_neighbor[0] + u->dy_neighbor[0]);
}

private:
double const_by_laplacian;
};

/// Constructor.
///
/// For the equation \f$ -K \Delta u = f \f$, the argument \c const_by_laplacian is equal to \$ K \$.
///
BasicKellyAdapt(std::vector<SpaceSharedPtr<Scalar> > spaces_,
double const_by_laplacian = 1.0,
std::vector<NormType> norms_ = std::vector<NormType>())
: KellyTypeAdapt<Scalar>(spaces_, true, std::vector<const InterfaceEstimatorScalingFunction*>(), norms_)
{
set_scaling_consts(const_by_laplacian);
for (int i = 0; i < this->num; i++)
this->error_estimators_surf.push_back(new ErrorEstimatorFormKelly(i, const_by_laplacian));
}

BasicKellyAdapt(SpaceSharedPtr<Scalar> space_, double const_by_laplacian = 1.0, NormType norm_ = HERMES_UNSET_NORM)
: KellyTypeAdapt<Scalar>(space_, true, nullptr, norm_)
{
set_scaling_consts(const_by_laplacian);
this->error_estimators_surf.push_back(new ErrorEstimatorFormKelly(0, const_by_laplacian));
}

private:
void set_scaling_consts(double C)
{
this->interface_scaling_const = 1./(24.*C);
this->volumetric_scaling_const = this->interface_scaling_const;
this->boundary_scaling_const = this->interface_scaling_const;
}
};
}
}
#endif
*/