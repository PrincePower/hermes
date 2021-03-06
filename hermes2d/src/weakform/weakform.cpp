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

#include "global.h"
#include "api2d.h"
#include "weakform.h"
#include "matrix.h"
#include "forms.h"
#include "shapeset_l2_all.h"
#include "shapeset_hc_all.h"
#include "shapeset_hd_all.h"
#include "shapeset_h1_all.h"
#include "algebra/dense_matrix_operations.h"

using namespace Hermes::Algebra::DenseMatrixOperations;

namespace Hermes
{
  namespace Hermes2D
  {
    template<typename Scalar>
    WeakFormSharedPtr<Scalar>::WeakFormSharedPtr(Hermes::Hermes2D::WeakForm<Scalar>* ptr) : std::tr1::shared_ptr<Hermes::Hermes2D::WeakForm<Scalar> >(ptr)
    {
    }

    template<typename Scalar>
    WeakFormSharedPtr<Scalar>::WeakFormSharedPtr(const WeakFormSharedPtr& other) : std::tr1::shared_ptr<Hermes::Hermes2D::WeakForm<Scalar> >(other)
    {
    }

    template<typename Scalar>
    void WeakFormSharedPtr<Scalar>::operator=(const WeakFormSharedPtr& other)
    {
      std::tr1::shared_ptr<Hermes::Hermes2D::WeakForm<Scalar> >::operator=(other);
    }

    static bool warned_nonOverride = false;

    // This is to be used by weak forms specifying numerical flux through interior edges.
    // Forms with this identifier will receive DiscontinuousFunc representations of shape
    // and ext. functions, which they may query for values on either side of given interface.
    static const std::string H2D_DG_INNER_EDGE = "-1234567";

    template<typename Scalar>
    WeakForm<Scalar>::WeakForm(unsigned int neq, bool mat_free) : Hermes::Mixins::Loggable(false)
    {
      this->neq = neq;
      this->original_neq = neq;
      this->is_matfree = mat_free;
    }

    template<typename Scalar>
    void WeakForm<Scalar>::free_ext()
    {
      this->ext.clear();
    }

    template<typename Scalar>
    WeakForm<Scalar>::~WeakForm()
    {
      for (unsigned int i = 0; i < this->forms.size(); i++)
        delete get_forms()[i];
      delete_all();
    }

    template<typename Scalar>
    WeakForm<Scalar>* WeakForm<Scalar>::clone() const
    {
      if (!warned_nonOverride)
#pragma omp critical (warning_weakform_nonOverride)
      {
        if (!warned_nonOverride)
          this->warn("Using default WeakForm<Scalar>::clone, if you have any dynamically created data in your WeakForm constructor, you need to overload this method!");
        warned_nonOverride = true;
      }
      return new WeakForm(*this);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_active_state(Element** e)
    {
      // Nothing here, supposed to be overriden if necessary.
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_active_edge_state(Element** e, unsigned char isurf)
    {
      // Nothing here, supposed to be overriden if necessary.
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_active_DG_state(Element** e, unsigned char isurf)
    {
      // Nothing here, supposed to be overriden if necessary.
    }

    template<typename Scalar>
    void WeakForm<Scalar>::cloneMembers(const WeakFormSharedPtr<Scalar>& other_wf)
    {
      this->mfvol.clear();
      this->mfsurf.clear();
      this->mfDG.clear();
      this->vfvol.clear();
      this->vfsurf.clear();
      this->vfDG.clear();
      this->forms.clear();
      this->ext.clear();

      for (unsigned int i = 0; i < other_wf->forms.size(); i++)
      {
        if (dynamic_cast<MatrixFormVol<Scalar>*>(other_wf->forms[i]) != nullptr)
        {
          MatrixFormVol<Scalar>* form = (dynamic_cast<MatrixFormVol<Scalar>*>(other_wf->forms[i]))->clone();
          form->copy_base(other_wf->forms[i]);
          this->forms.push_back(form);
          this->mfvol.push_back(dynamic_cast<MatrixFormVol<Scalar>*>(this->forms.back()));
          continue;
        }
        if (dynamic_cast<VectorFormVol<Scalar>*>(other_wf->forms[i]) != nullptr)
        {
          VectorFormVol<Scalar>* form = (dynamic_cast<VectorFormVol<Scalar>*>(other_wf->forms[i]))->clone();
          form->copy_base(other_wf->forms[i]);
          this->forms.push_back(form);
          this->vfvol.push_back(dynamic_cast<VectorFormVol<Scalar>*>(this->forms.back()));
          continue;
        }
        if (dynamic_cast<MatrixFormSurf<Scalar>*>(other_wf->forms[i]) != nullptr)
        {
          MatrixFormSurf<Scalar>* form = (dynamic_cast<MatrixFormSurf<Scalar>*>(other_wf->forms[i]))->clone();
          form->copy_base(other_wf->forms[i]);
          this->forms.push_back(form);
          this->mfsurf.push_back(dynamic_cast<MatrixFormSurf<Scalar>*>(this->forms.back()));
          continue;
        }
        if (dynamic_cast<VectorFormSurf<Scalar>*>(other_wf->forms[i]) != nullptr)
        {
          VectorFormSurf<Scalar>* form = (dynamic_cast<VectorFormSurf<Scalar>*>(other_wf->forms[i]))->clone();
          form->copy_base(other_wf->forms[i]);
          this->forms.push_back(form);
          this->vfsurf.push_back(dynamic_cast<VectorFormSurf<Scalar>*>(this->forms.back()));
          continue;
        }
        if (dynamic_cast<MatrixFormDG<Scalar>*>(other_wf->forms[i]) != nullptr)
        {
          MatrixFormDG<Scalar>* form = (dynamic_cast<MatrixFormDG<Scalar>*>(other_wf->forms[i]))->clone();
          form->copy_base(other_wf->forms[i]);
          this->forms.push_back(form);
          this->mfDG.push_back(dynamic_cast<MatrixFormDG<Scalar>*>(this->forms.back()));
          continue;
        }
        if (dynamic_cast<VectorFormDG<Scalar>*>(other_wf->forms[i]) != nullptr)
        {
          VectorFormDG<Scalar>* form = (dynamic_cast<VectorFormDG<Scalar>*>(other_wf->forms[i]))->clone();
          form->copy_base(other_wf->forms[i]);
          this->forms.push_back(form);
          this->forms.push_back((dynamic_cast<VectorFormDG<Scalar>*>(other_wf->forms[i]))->clone());
          this->vfDG.push_back(dynamic_cast<VectorFormDG<Scalar>*>(this->forms.back()));
          continue;
        }
      }
      for (unsigned int i = 0; i < other_wf->forms.size(); i++)
      {
        this->cloneMemberExtFunctions(other_wf->forms[i]->ext, this->forms[i]->ext);
        other_wf->forms[i]->u_ext_fn = this->forms[i]->u_ext_fn;
        this->forms[i]->wf = this;
      }
      this->cloneMemberExtFunctions(other_wf->ext, this->ext);
      this->u_ext_fn = other_wf->u_ext_fn;
    }

    template<typename Scalar>
    void WeakForm<Scalar>::cloneMemberExtFunctions(std::vector<MeshFunctionSharedPtr<Scalar> > source_ext, std::vector<MeshFunctionSharedPtr<Scalar> >& cloned_ext)
    {
      cloned_ext.clear();
      for (unsigned int i = 0; i < source_ext.size(); i++)
      {
        Solution<Scalar>* originalSln = dynamic_cast<Solution<Scalar>*>(source_ext[i].get());
        if (originalSln)
        {
          Solution<Scalar>* newSln = nullptr;
          if (originalSln->get_type() == HERMES_SLN)
          {
            newSln = new Solution < Scalar > ;
            newSln->copy(source_ext[i].get());
          }
          else
            newSln = static_cast<Solution<Scalar>*>(originalSln->clone());

          newSln->set_type(originalSln->get_type());
          cloned_ext.push_back(newSln);
        }
        else
          cloned_ext.push_back(source_ext[i]->clone());
      }
    }

    template<typename Scalar>
    void WeakForm<Scalar>::delete_all()
    {
      mfvol.clear();
      mfsurf.clear();
      mfDG.clear();
      vfvol.clear();
      vfsurf.clear();
      vfDG.clear();
      forms.clear();
    };

    template<typename Scalar>
    void WeakForm<Scalar>::set_ext(MeshFunctionSharedPtr<Scalar> ext)
    {
      this->ext.clear();
      this->ext.push_back(ext);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_ext(std::vector<MeshFunctionSharedPtr<Scalar> > ext)
    {
      this->ext = ext;
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_u_ext_fn(UExtFunctionSharedPtr<Scalar> ext)
    {
      this->u_ext_fn.clear();
      this->u_ext_fn.push_back(ext);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_u_ext_fn(std::vector<UExtFunctionSharedPtr<Scalar> > ext)
    {
      this->u_ext_fn = ext;
    }

    template<typename Scalar>
    template<typename FormType>
    void WeakForm<Scalar>::processFormMarkers(const std::vector<SpaceSharedPtr<Scalar> > spaces, bool surface, std::vector<FormType> forms_to_process)
    {
      for (unsigned short form_i = 0; form_i < forms_to_process.size(); form_i++)
      {
        Form<Scalar>* form = forms_to_process[form_i];
        form->areas_internal.clear();
        for (unsigned short marker_i = 0; marker_i < form->areas.size(); marker_i++)
        {
          if (form->areas[marker_i] == HERMES_ANY)
          {
            form->assembleEverywhere = true;
            form->areas_internal.clear();
            break;
          }

          Mesh::MarkersConversion::IntValid marker;
          if (surface)
            marker = spaces[form->i]->get_mesh()->get_boundary_markers_conversion().get_internal_marker(form->areas[marker_i]);
          else
            marker = spaces[form->i]->get_mesh()->get_element_markers_conversion().get_internal_marker(form->areas[marker_i]);

          if (marker.valid)
            form->areas_internal.push_back(marker.marker);
          else
            throw Exceptions::Exception("Marker not valid in assembling: %s.", form->areas[marker_i].c_str());
        }
      }
    }

    template<typename Scalar>
    void WeakForm<Scalar>::processFormMarkers(const std::vector<SpaceSharedPtr<Scalar> > spaces)
    {
      processFormMarkers(spaces, false, this->mfvol);
      processFormMarkers(spaces, false, this->vfvol);
      processFormMarkers(spaces, true, this->mfsurf);
      processFormMarkers(spaces, true, this->vfsurf);
    }

    template<typename Scalar>
    bool WeakForm<Scalar>::is_DG() const
    {
      if (this->mfDG.empty() && this->vfDG.empty())
        return false;
      return true;
    }

    template<typename Scalar>
    std::vector<MeshFunctionSharedPtr<Scalar> > WeakForm<Scalar>::get_ext() const
    {
      return this->ext;
    }

    template<typename Scalar>
    Form<Scalar>::Form(int i) : scaling_factor(1.0), wf(nullptr), assembleEverywhere(false), i(i)
    {
      areas.push_back(HERMES_ANY);
      stage_time = 0.0;
    }

    template<typename Scalar>
    Form<Scalar>::~Form()
    {
    }

    template<typename Scalar>
    void Form<Scalar>::set_current_stage_time(double time)
    {
      stage_time = time;
    }

    template<typename Scalar>
    double Form<Scalar>::get_current_stage_time() const
    {
      return stage_time;
    }

    template<typename Scalar>
    void Form<Scalar>::set_area(std::string area)
    {
      areas.clear();
      areas.push_back(area);
    }
    template<typename Scalar>
    void Form<Scalar>::set_areas(std::vector<std::string> areas)
    {
      this->areas = areas;
    }

    template<typename Scalar>
    std::vector<std::string> Form<Scalar>::getAreas() const
    {
      return this->areas;
    }

    template<typename Scalar>
    void Form<Scalar>::setScalingFactor(double scalingFactor)
    {
      this->scaling_factor = scalingFactor;
    }

    template<typename Scalar>
    void Form<Scalar>::set_ext(MeshFunctionSharedPtr<Scalar> ext)
    {
      this->ext.clear();
      this->ext.push_back(ext);
    }

    template<typename Scalar>
    void Form<Scalar>::set_ext(std::vector<MeshFunctionSharedPtr<Scalar> > ext)
    {
      this->ext = ext;
    }

    template<typename Scalar>
    void Form<Scalar>::set_u_ext_fn(UExtFunctionSharedPtr<Scalar> ext)
    {
      this->u_ext_fn.clear();
      this->u_ext_fn.push_back(ext);
    }

    template<typename Scalar>
    void Form<Scalar>::set_u_ext_fn(std::vector<UExtFunctionSharedPtr<Scalar> > ext)
    {
      this->u_ext_fn = ext;
    }

    template<typename Scalar>
    std::vector<MeshFunctionSharedPtr<Scalar> > Form<Scalar>::get_ext() const
    {
      return this->ext;
    }

    template<typename Scalar>
    void Form<Scalar>::copy_base(Form<Scalar>* other_form)
    {
      this->stage_time = other_form->stage_time;
      this->scaling_factor = other_form->scaling_factor;
      this->u_ext_offset = other_form->u_ext_offset;
      this->previous_iteration_space_index = other_form->previous_iteration_space_index;
    }

    template<typename Scalar>
    MatrixForm<Scalar>::MatrixForm(unsigned int i, unsigned int j) :
      Form<Scalar>(i), sym(HERMES_NONSYM), j(j)
    {
      this->previous_iteration_space_index = j;
    }

    template<typename Scalar>
    MatrixForm<Scalar>::~MatrixForm()
    {
    }

    template<typename Scalar>
    MatrixFormVol<Scalar>::MatrixFormVol(unsigned int i, unsigned int j) :
      MatrixForm<Scalar>(i, j)
    {
    }

    template<typename Scalar>
    MatrixFormVol<Scalar>::~MatrixFormVol()
    {
    }

    template<typename Scalar>
    void MatrixFormVol<Scalar>::setSymFlag(SymFlag sym)
    {
      this->sym = sym;
    }

    template<typename Scalar>
    SymFlag MatrixFormVol<Scalar>::getSymFlag() const
    {
      return this->sym;
    }

    template<typename Scalar>
    Scalar MatrixFormVol<Scalar>::value(int n, double *wt, Func<Scalar> **u_ext, Func<double> *u, Func<double> *v,
      GeomVol<double> *e, Func<Scalar> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormVol<Scalar>::value");
      return 0.0;
    }

    template<typename Scalar>
    Hermes::Ord MatrixFormVol<Scalar>::ord(int n, double *wt, Func<Hermes::Ord> **u_ext, Func<Hermes::Ord> *u, Func<Hermes::Ord> *v,
      GeomVol<Hermes::Ord> *e, Func<Ord> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormVol<Scalar>::ord");
      return Hermes::Ord();
    }

    template<typename Scalar>
    MatrixFormVol<Scalar>* MatrixFormVol<Scalar>::clone() const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormVol<Scalar>::clone()");
      return nullptr;
    }

    template<typename Scalar>
    MatrixFormSurf<Scalar>::MatrixFormSurf(unsigned int i, unsigned int j) :
      MatrixForm<Scalar>(i, j)
    {
    }

    template<typename Scalar>
    Scalar MatrixFormSurf<Scalar>::value(int n, double *wt, Func<Scalar> **u_ext, Func<double> *u, Func<double> *v,
      GeomSurf<double> *e, Func<Scalar> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormSurf<Scalar>::value");
      return 0.0;
    }

    template<typename Scalar>
    Hermes::Ord MatrixFormSurf<Scalar>::ord(int n, double *wt, Func<Hermes::Ord> **u_ext, Func<Hermes::Ord> *u, Func<Hermes::Ord> *v,
      GeomSurf<Hermes::Ord> *e, Func<Ord> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormSurf<Scalar>::ord");
      return Hermes::Ord();
    }

    template<typename Scalar>
    MatrixFormSurf<Scalar>* MatrixFormSurf<Scalar>::clone() const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormSurf<Scalar>::clone()");
      return nullptr;
    }

    template<typename Scalar>
    MatrixFormSurf<Scalar>::~MatrixFormSurf()
    {
    }

    template<typename Scalar>
    MatrixFormDG<Scalar>::MatrixFormDG(unsigned int i, unsigned int j) :
      Form<Scalar>(i), j(j)
    {
      this->previous_iteration_space_index = j;
      this->set_area(H2D_DG_INNER_EDGE);
    }

    template<typename Scalar>
    MatrixFormDG<Scalar>::~MatrixFormDG()
    {
    }

    template<typename Scalar>
    Scalar MatrixFormDG<Scalar>::value(int n, double *wt, DiscontinuousFunc<Scalar> **u_ext, DiscontinuousFunc<double> *u, DiscontinuousFunc<double> *v,
      InterfaceGeom<double> *e, DiscontinuousFunc<Scalar> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormDG<Scalar>::value");
      return 0.0;
    }

    template<typename Scalar>
    Hermes::Ord MatrixFormDG<Scalar>::ord(int n, double *wt, DiscontinuousFunc<Hermes::Ord> **u_ext, DiscontinuousFunc<Hermes::Ord> *u, DiscontinuousFunc<Hermes::Ord> *v,
      InterfaceGeom<Hermes::Ord> *e, DiscontinuousFunc<Ord> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormDG<Scalar>::ord");
      return Hermes::Ord();
    }

    template<typename Scalar>
    MatrixFormDG<Scalar>* MatrixFormDG<Scalar>::clone() const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("MatrixFormDG<Scalar>::clone()");
      return nullptr;
    }

    template<typename Scalar>
    VectorForm<Scalar>::VectorForm(unsigned int i) :
      Form<Scalar>(i)
    {
      this->previous_iteration_space_index = i;
    }

    template<typename Scalar>
    VectorForm<Scalar>::~VectorForm()
    {
    }

    template<typename Scalar>
    VectorFormVol<Scalar>::VectorFormVol(unsigned int i) :
      VectorForm<Scalar>(i)
    {
    }

    template<typename Scalar>
    VectorFormVol<Scalar>::~VectorFormVol()
    {
    }

    template<typename Scalar>
    Scalar VectorFormVol<Scalar>::value(int n, double *wt, Func<Scalar> **u_ext, Func<double> *v,
      GeomVol<double> *e, Func<Scalar> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormVol<Scalar>::value");
      return 0.0;
    }

    template<typename Scalar>
    Hermes::Ord VectorFormVol<Scalar>::ord(int n, double *wt, Func<Hermes::Ord> **u_ext, Func<Hermes::Ord> *v,
      GeomVol<Hermes::Ord> *e, Func<Ord> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormVol<Scalar>::ord");
      return Hermes::Ord();
    }

    template<typename Scalar>
    VectorFormVol<Scalar>* VectorFormVol<Scalar>::clone() const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormVol<Scalar>::clone()");
      return nullptr;
    }

    template<typename Scalar>
    VectorFormSurf<Scalar>::VectorFormSurf(unsigned int i) :
      VectorForm<Scalar>(i)
    {
    }

    template<typename Scalar>
    Scalar VectorFormSurf<Scalar>::value(int n, double *wt, Func<Scalar> **u_ext, Func<double> *v,
      GeomSurf<double> *e, Func<Scalar> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormSurf<Scalar>::value");
      return 0.0;
    }

    template<typename Scalar>
    Hermes::Ord VectorFormSurf<Scalar>::ord(int n, double *wt, Func<Hermes::Ord> **u_ext, Func<Hermes::Ord> *v,
      GeomSurf<Hermes::Ord> *e, Func<Ord> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormSurf<Scalar>::ord");
      return Hermes::Ord();
    }

    template<typename Scalar>
    VectorFormSurf<Scalar>::~VectorFormSurf()
    {
    }

    template<typename Scalar>
    VectorFormSurf<Scalar>* VectorFormSurf<Scalar>::clone() const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormSurf<Scalar>::clone()");
      return nullptr;
    }

    template<typename Scalar>
    VectorFormDG<Scalar>::VectorFormDG(unsigned int i) :
      Form<Scalar>(i)
    {
      this->set_area(H2D_DG_INNER_EDGE);
    }

    template<typename Scalar>
    VectorFormDG<Scalar>::~VectorFormDG()
    {
    }

    template<typename Scalar>
    Scalar VectorFormDG<Scalar>::value(int n, double *wt, DiscontinuousFunc<Scalar> **u_ext, Func<double> *v,
      InterfaceGeom<double> *e, DiscontinuousFunc<Scalar> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormDG<Scalar>::value");
      return 0.0;
    }

    template<typename Scalar>
    Hermes::Ord VectorFormDG<Scalar>::ord(int n, double *wt, DiscontinuousFunc<Hermes::Ord> **u_ext, Func<Hermes::Ord> *v,
      InterfaceGeom<Hermes::Ord> *e, DiscontinuousFunc<Ord> **ext) const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormDG<Scalar>::ord");
      return Hermes::Ord();
    }

    template<typename Scalar>
    VectorFormDG<Scalar>* VectorFormDG<Scalar>::clone() const
    {
      throw Hermes::Exceptions::MethodNotOverridenException("VectorFormDG<Scalar>::clone()");
      return nullptr;
    }

    template<typename Scalar>
    void Form<Scalar>::set_weakform(WeakForm<Scalar>* wf)
    {
      this->wf = wf;
      if (wf->original_neq != wf->neq)
        this->previous_iteration_space_index %= wf->original_neq;
    }

    template<typename Scalar>
    void WeakForm<Scalar>::add_matrix_form(MatrixFormVol<Scalar>* form)
    {
      if (form->i >= neq || form->j >= neq)
        throw Hermes::Exceptions::Exception("Invalid equation number.");
      if (form->sym < -1 || form->sym > 1)
        throw Hermes::Exceptions::Exception("\"sym\" must be -1, 0 or 1.");
      if (form->sym < 0 && form->i == form->j)
        throw Hermes::Exceptions::Exception("Only off-diagonal forms can be antisymmetric.");
      if (mfvol.size() > 100)
      {
        this->warn("Large number of forms (> 100). Is this the intent?");
      }

      form->set_weakform(this);
      mfvol.push_back(form);
      forms.push_back(form);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::add_matrix_form_surf(MatrixFormSurf<Scalar>* form)
    {
      if (form->i >= neq || form->j >= neq)
        throw Hermes::Exceptions::Exception("Invalid equation number.");

      form->set_weakform(this);
      mfsurf.push_back(form);
      forms.push_back(form);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::add_matrix_form_DG(MatrixFormDG<Scalar>* form)
    {
      if (form->i >= neq || form->j >= neq)
        throw Hermes::Exceptions::Exception("Invalid equation number.");

      form->set_weakform(this);
      mfDG.push_back(form);
      forms.push_back(form);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::add_vector_form(VectorFormVol<Scalar>* form)
    {
      if (form->i >= neq)
        throw Hermes::Exceptions::Exception("Invalid equation number.");
      form->set_weakform(this);
      vfvol.push_back(form);
      forms.push_back(form);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::add_vector_form_surf(VectorFormSurf<Scalar>* form)
    {
      if (form->i >= neq)
        throw Hermes::Exceptions::Exception("Invalid equation number.");

      form->set_weakform(this);
      vfsurf.push_back(form);
      forms.push_back(form);
    }

    template<typename Scalar>
    void WeakForm<Scalar>::add_vector_form_DG(VectorFormDG<Scalar>* form)
    {
      if (form->i >= neq)
        throw Hermes::Exceptions::Exception("Invalid equation number.");

      form->set_weakform(this);
      vfDG.push_back(form);
      forms.push_back(form);
    }

    template<typename Scalar>
    std::vector<Form<Scalar> *> WeakForm<Scalar>::get_forms() const
    {
      return forms;
    }

    template<typename Scalar>
    std::vector<MatrixFormVol<Scalar> *> WeakForm<Scalar>::get_mfvol() const
    {
      return mfvol;
    }
    template<typename Scalar>
    std::vector<MatrixFormSurf<Scalar> *> WeakForm<Scalar>::get_mfsurf() const
    {
      return mfsurf;
    }
    template<typename Scalar>
    std::vector<MatrixFormDG<Scalar> *> WeakForm<Scalar>::get_mfDG() const
    {
      return mfDG;
    }
    template<typename Scalar>
    std::vector<VectorFormVol<Scalar> *> WeakForm<Scalar>::get_vfvol() const
    {
      return vfvol;
    }
    template<typename Scalar>
    std::vector<VectorFormSurf<Scalar> *> WeakForm<Scalar>::get_vfsurf() const
    {
      return vfsurf;
    }
    template<typename Scalar>
    std::vector<VectorFormDG<Scalar> *> WeakForm<Scalar>::get_vfDG() const
    {
      return vfDG;
    }

    template<typename Scalar>
    bool** WeakForm<Scalar>::get_blocks(bool force_diagonal_blocks) const
    {
      bool** blocks = new_matrix<bool>(neq, neq);
      for (unsigned int i = 0; i < neq; i++)
      {
        for (unsigned int j = 0; j < neq; j++)
          blocks[i][j] = false;
        if (force_diagonal_blocks)
          blocks[i][i] = true;
      }
      for (unsigned i = 0; i < mfvol.size(); i++)
      {
        if (fabs(mfvol[i]->scaling_factor) > Hermes::HermesSqrtEpsilon)
          blocks[mfvol[i]->i][mfvol[i]->j] = true;
        if (mfvol[i]->sym)
          if (fabs(mfvol[i]->scaling_factor) > Hermes::HermesSqrtEpsilon)
            blocks[mfvol[i]->j][mfvol[i]->i] = true;
      }
      for (unsigned i = 0; i < mfsurf.size(); i++)
      {
        if (fabs(mfsurf[i]->scaling_factor) > Hermes::HermesSqrtEpsilon)
          blocks[mfsurf[i]->i][mfsurf[i]->j] = true;
      }
      for (unsigned i = 0; i < mfDG.size(); i++)
      {
        if (fabs(mfDG[i]->scaling_factor) > Hermes::HermesSqrtEpsilon)
          blocks[mfDG[i]->i][mfDG[i]->j] = true;
      }

      return blocks;
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_current_time(double time)
    {
      current_time = time;
    }

    template<typename Scalar>
    double WeakForm<Scalar>::get_current_time() const
    {
      return current_time;
    }

    template<typename Scalar>
    void WeakForm<Scalar>::set_current_time_step(double time_step)
    {
      current_time_step = time_step;
    }

    template<typename Scalar>
    double WeakForm<Scalar>::get_current_time_step() const
    {
      return current_time_step;
    }

    template class HERMES_API WeakForm < double > ;
    template class HERMES_API WeakForm < std::complex<double> > ;
    template class HERMES_API Form < double > ;
    template class HERMES_API Form < std::complex<double> > ;
    template class HERMES_API MatrixForm < double > ;
    template class HERMES_API MatrixForm < std::complex<double> > ;
    template class HERMES_API MatrixFormVol < double > ;
    template class HERMES_API MatrixFormVol < std::complex<double> > ;
    template class HERMES_API MatrixFormSurf < double > ;
    template class HERMES_API MatrixFormSurf < std::complex<double> > ;
    template class HERMES_API MatrixFormDG < double > ;
    template class HERMES_API MatrixFormDG < std::complex<double> > ;
    template class HERMES_API VectorForm < double > ;
    template class HERMES_API VectorForm < std::complex<double> > ;
    template class HERMES_API VectorFormVol < double > ;
    template class HERMES_API VectorFormVol < std::complex<double> > ;
    template class HERMES_API VectorFormSurf < double > ;
    template class HERMES_API VectorFormSurf < std::complex<double> > ;
    template class HERMES_API VectorFormDG < double > ;
    template class HERMES_API VectorFormDG < std::complex<double> > ;

    template class HERMES_API WeakFormSharedPtr < double > ;
    template class HERMES_API WeakFormSharedPtr < std::complex<double> > ;
  }
}
