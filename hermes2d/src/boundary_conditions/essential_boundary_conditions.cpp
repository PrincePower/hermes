// This file is part of Hermes2D
//
// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Email: hpfem-group@unr.edu, home page: http://www.hpfem.org/.
//
// Hermes2D is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "essential_boundary_conditions.h"
#include "exact_solution.h"
namespace Hermes
{
  namespace Hermes2D
  {
    template<typename Scalar>
    EssentialBoundaryCondition<Scalar>::EssentialBoundaryCondition()
    {
      current_time = 0.0;
      value_const = 0.0;
    }

    template<typename Scalar>
    EssentialBoundaryCondition<Scalar>::EssentialBoundaryCondition(std::vector<std::string> markers) : markers(markers)
    {
      current_time = 0.0;
      value_const = 0.0;
    }

    template<typename Scalar>
    EssentialBoundaryCondition<Scalar>::EssentialBoundaryCondition(std::string marker)
    {
      markers.push_back(marker);
      current_time = 0.0;
      value_const = 0.0;
    }

    template<typename Scalar>
    EssentialBoundaryCondition<Scalar>::~EssentialBoundaryCondition()
    {
    }

    template<typename Scalar>
    void EssentialBoundaryCondition<Scalar>::set_current_time(double time)
    {
      this->current_time = time;
    }

    template<typename Scalar>
    double EssentialBoundaryCondition<Scalar>::get_current_time() const
    {
      return current_time;
    }

    template<typename Scalar>
    DefaultEssentialBCConst<Scalar>::DefaultEssentialBCConst(std::vector<std::string> markers, Scalar value_const) : EssentialBoundaryCondition<Scalar>(markers)
    {
      this->value_const = value_const;
    }

    template<typename Scalar>
    DefaultEssentialBCConst<Scalar>::DefaultEssentialBCConst(std::string marker, Scalar value_const) : EssentialBoundaryCondition<Scalar>()
    {
      this->value_const = value_const;
      this->markers.push_back(marker);
    }

    template<typename Scalar>
    Scalar DefaultEssentialBCConst<Scalar>::value(double x, double y) const
    {
      this->warn("EssentialBoundaryCondition::Function used either for a constant condition, or not redefined for nonconstant condition.");
      return 0.0;
    }

    template<typename Scalar>
    DefaultEssentialBCNonConst<Scalar>::DefaultEssentialBCNonConst(std::vector<std::string> markers_,
      MeshFunctionSharedPtr<Scalar> exact_solution_)
      : EssentialBoundaryCondition<Scalar>(std::vector<std::string>())
    {
      ExactSolutionScalar<Scalar>* exact = dynamic_cast<ExactSolutionScalar<Scalar>*>(exact_solution_.get());
      if (exact)
        this->exact_solution = exact;
      else
        throw Exceptions::Exception("Wrong mesh function type passed to DefaultEssentialBCNonConst.");

      for (unsigned int i = 0; i < markers_.size(); i++)
        this->markers.push_back(markers_[i]);
    }

    template<typename Scalar>
    DefaultEssentialBCNonConst<Scalar>::DefaultEssentialBCNonConst(std::string marker, MeshFunctionSharedPtr<Scalar> exact_solution_)
      : EssentialBoundaryCondition<Scalar>(std::vector<std::string>())
    {
      ExactSolutionScalar<Scalar>* exact = dynamic_cast<ExactSolutionScalar<Scalar>*>(exact_solution_.get());
      if (exact)
        this->exact_solution = exact;
      else
        throw Exceptions::Exception("Wrong mesh function type passed to DefaultEssentialBCNonConst.");
      this->markers.push_back(marker);
    }

    template<typename Scalar>
    Scalar DefaultEssentialBCNonConst<Scalar>::value(double x, double y) const
    {
      return exact_solution->value(x, y);
    }

    template<typename Scalar>
    EssentialBCs<Scalar>::EssentialBCs() : HermesAnyBC(nullptr)
    {
    }

    template<typename Scalar>
    EssentialBCs<Scalar>::EssentialBCs(std::vector<EssentialBoundaryCondition<Scalar> *> essential_bcs) : HermesAnyBC(nullptr)
    {
      add_boundary_conditions(essential_bcs);
    }

    template<typename Scalar>
    EssentialBCs<Scalar>::EssentialBCs(EssentialBoundaryCondition<Scalar> * boundary_condition) : HermesAnyBC(nullptr)
    {
      std::vector<EssentialBoundaryCondition<Scalar> *> boundary_conditions;
      boundary_conditions.push_back(boundary_condition);
      add_boundary_conditions(boundary_conditions);
    }

    template<typename Scalar>
    void EssentialBCs<Scalar>::add_boundary_conditions(std::vector<EssentialBoundaryCondition<Scalar> *> boundary_conditions)
    {
      for (typename std::vector<EssentialBoundaryCondition<Scalar> *>::iterator it = boundary_conditions.begin(); it != boundary_conditions.end(); it++)
        all.push_back(*it);

      this->markers.clear();
      create_marker_cache();
    }

    template<typename Scalar>
    void EssentialBCs<Scalar>::add_boundary_condition(EssentialBoundaryCondition<Scalar> * boundary_condition)
    {
      std::vector<EssentialBoundaryCondition<Scalar> *> boundary_conditions;
      boundary_conditions.push_back(boundary_condition);
      add_boundary_conditions(boundary_conditions);
    }

    template<typename Scalar>
    typename std::vector<EssentialBoundaryCondition<Scalar> *>::const_iterator EssentialBCs<Scalar>::begin() const
    {
      return all.begin();
    }

    template<typename Scalar>
    typename std::vector<EssentialBoundaryCondition<Scalar> *>::const_iterator EssentialBCs<Scalar>::end() const
    {
      return all.end();
    }

    template<typename Scalar>
    EssentialBCs<Scalar>::~EssentialBCs()
    {
    }

    template<typename Scalar>
    void EssentialBCs<Scalar>::create_marker_cache()
    {
      bool hermes_any_set = false;
      this->markers.clear();
      this->BCs.clear();
      EssentialBoundaryCondition<Scalar>* any_set = nullptr;
      for (this->iterator = begin(); iterator != end(); ++iterator)
        for (std::vector<std::string>::const_iterator it = (*iterator)->markers.begin(); it != (*iterator)->markers.end(); ++it)
        {
        if (hermes_any_set)
          throw Hermes::Exceptions::Exception("Attempt to define a BC on HERMES_ANY together with a BC on a specific part: '%s'.", it->c_str());
        if ((*it) == HERMES_ANY)
        {
          if (any_set != nullptr)
            throw Hermes::Exceptions::Exception("Attempt to define a BC on HERMES_ANY together with a BC on a specific part: '%s'.", any_set->markers.begin()->c_str());
          hermes_any_set = true;
          this->HermesAnyBC = *iterator;
        }
        else
        {
          any_set = *iterator;
          for (unsigned short i = 0; i < this->markers.size(); i++)
            if (this->markers[i] == *it)
              throw Hermes::Exceptions::Exception("Attempt to define more than one description of the BC on the same part of the boundary with marker '%s'.", it->c_str());
          this->markers.push_back(*it);
          this->BCs.push_back(*iterator);
        }
        }
    }

    template<typename Scalar>
    const std::vector<std::string>& EssentialBCs<Scalar>::get_markers() const
    {
      return this->markers;
    }

    template<typename Scalar>
    EssentialBoundaryCondition<Scalar>* EssentialBCs<Scalar>::get_boundary_condition(std::string marker)
    {
      if (this->HermesAnyBC != nullptr)
        return this->HermesAnyBC;
      for (unsigned short i = 0; i < this->markers.size(); i++)
        if (this->markers[i] == marker)
          return this->BCs[i];
      return nullptr;
    }

    template<typename Scalar>
    void EssentialBCs<Scalar>::set_current_time(double time)
    {
      for (iterator = begin(); iterator != end(); iterator++)
        (*iterator)->set_current_time(time);
    }

    template HERMES_API class EssentialBoundaryCondition < double > ;
    template HERMES_API class EssentialBoundaryCondition < std::complex<double> > ;
    template HERMES_API class DefaultEssentialBCConst < double > ;
    template HERMES_API class DefaultEssentialBCConst < std::complex<double> > ;
    template HERMES_API class DefaultEssentialBCNonConst < double > ;
    template HERMES_API class DefaultEssentialBCNonConst < std::complex<double> > ;
    template HERMES_API class EssentialBCs < double > ;
    template HERMES_API class EssentialBCs < std::complex<double> > ;
  }
}