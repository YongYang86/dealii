//---------------------------------------------------------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//---------------------------------------------------------------------------

#include <base/quadrature.h>
#include <base/template_constraints.h>
#include <fe/fe_dgq.h>
#include <fe/fe_tools.h>


#include <iostream>
#include <sstream>

DEAL_II_NAMESPACE_OPEN


// namespace for some functions that are used in this file. they are
// specific to numbering conventions used for the FE_DGQ element, and
// are thus not very interesting to the outside world
namespace
{
				   // given an integer N, compute its
				   // integer square root (if it
				   // exists, otherwise give up)
  inline unsigned int int_sqrt (const unsigned int N)
  {
    for (unsigned int i=0; i<=N; ++i)
      if (i*i == N)
	return i;
    Assert (false, ExcInternalError());
    return deal_II_numbers::invalid_unsigned_int;
  }


				   // given an integer N, compute its
				   // integer cube root (if it
				   // exists, otherwise give up)
  inline unsigned int int_cuberoot (const unsigned int N)
  {
    for (unsigned int i=0; i<=N; ++i)
      if (i*i*i == N)
	return i;
    Assert (false, ExcInternalError());
    return deal_II_numbers::invalid_unsigned_int;
  }


				   // given N, generate i=0...N-1
				   // equidistant points in the
				   // interior of the interval [0,1]
  inline Point<1>
  generate_unit_point (const unsigned int i,
		       const unsigned int N,
		       const dealii::internal::int2type<1>  )
  {
    Assert (i<N, ExcInternalError());
    if (N==1)
      return Point<1> (.5);
    else
      {
        const double h = 1./(N-1);
        return Point<1>(i*h);
      }
  }
  
    
				   // given N, generate i=0...N-1
				   // equidistant points in the domain
				   // [0,1]^2
  inline Point<2>
  generate_unit_point (const unsigned int i,
		       const unsigned int N,
		       const dealii::internal::int2type<2>  )
  {
    Assert (i<N, ExcInternalError());
    
    if (N==1)
      return Point<2> (.5, .5);
    else
      {
        Assert (N>=4, ExcInternalError());
        const unsigned int N1d = int_sqrt(N);
        const double h = 1./(N1d-1);
        
        return Point<2> (i%N1d * h,
                         i/N1d * h);
      }
  }
  

  

				   // given N, generate i=0...N-1
				   // equidistant points in the domain
				   // [0,1]^3
  inline Point<3>
  generate_unit_point (const unsigned int i,
		       const unsigned int N,
		       const dealii::internal::int2type<3>  )
  {
    Assert (i<N, ExcInternalError());
    if (N==1)
      return Point<3> (.5, .5, .5);
    else
      {
        Assert (N>=8, ExcInternalError());
    
        const unsigned int N1d = int_cuberoot(N);
        const double h = 1./(N1d-1);

        return Point<3> (i%N1d * h,
                         (i/N1d)%N1d * h,
                         i/(N1d*N1d) * h);
      }
  }
}




template <int dim>
FE_DGQ<dim>::FE_DGQ (const unsigned int degree)
		:
		FE_Poly<TensorProductPolynomials<dim>, dim> (
		  TensorProductPolynomials<dim>(Polynomials::LagrangeEquidistant::generate_complete_basis(degree)),
		  FiniteElementData<dim>(get_dpo_vector(degree), 1, degree, FiniteElementData<dim>::L2),
		  std::vector<bool>(FiniteElementData<dim>(get_dpo_vector(degree),1, degree).dofs_per_cell, true),
		  std::vector<std::vector<bool> >(FiniteElementData<dim>(
		    get_dpo_vector(degree),1, degree).dofs_per_cell, std::vector<bool>(1,true)))
{
				   // Fill prolongation matrices with embedding operators
  for (unsigned int i=0; i<GeometryInfo<dim>::children_per_cell; ++i)
    this->prolongation[i].reinit (this->dofs_per_cell,
				  this->dofs_per_cell);
  FETools::compute_embedding_matrices (*this, this->prolongation);
				   // Fill restriction matrices with L2-projection
  for (unsigned int i=0; i<GeometryInfo<dim>::children_per_cell; ++i)
    this->restriction[i].reinit (this->dofs_per_cell,
				  this->dofs_per_cell);
  FETools::compute_projection_matrices (*this, this->restriction);
  
  
				   // finally fill in support points
  if (degree == 0)
    {
				       // constant elements, take
				       // midpoint
      this->unit_support_points.resize(1);
      for (unsigned int i=0; i<dim; ++i)
	this->unit_support_points[0](i) = 0.5;
    }
  else
    {
				       // number of points: (degree+1)^dim
      unsigned int n = degree+1;
      for (unsigned int i=1; i<dim; ++i)
	n *= degree+1;
      
      this->unit_support_points.resize(n);
      
      const double step = 1./degree;
      Point<dim> p;
      
      unsigned int k=0;
      for (unsigned int iz=0; iz <= ((dim>2) ? degree : 0) ; ++iz)
	for (unsigned int iy=0; iy <= ((dim>1) ? degree : 0) ; ++iy)
	  for (unsigned int ix=0; ix<=degree; ++ix)
	    {
	      p(0) = ix * step;
	      if (dim>1)
		p(1) = iy * step;
	      if (dim>2)
		p(2) = iz * step;
	      
	      this->unit_support_points[k++] = p;
	    };
    };

				   // note: no face support points for
				   // DG elements
}



template <int dim>
FE_DGQ<dim>::FE_DGQ (const Quadrature<1>& points)
		:
		FE_Poly<TensorProductPolynomials<dim>, dim> (
		  TensorProductPolynomials<dim>(Polynomials::Lagrange::generate_complete_basis(points.get_points())),
		  FiniteElementData<dim>(get_dpo_vector(points.n_quadrature_points-1), 1, points.n_quadrature_points-1, FiniteElementData<dim>::L2),
		  std::vector<bool>(FiniteElementData<dim>(get_dpo_vector(points.n_quadrature_points-1),1, points.n_quadrature_points-1).dofs_per_cell, true),
		  std::vector<std::vector<bool> >(FiniteElementData<dim>(
		    get_dpo_vector(points.n_quadrature_points-1),1, points.n_quadrature_points-1).dofs_per_cell, std::vector<bool>(1,true)))
{
  for (unsigned int i=0; i<GeometryInfo<dim>::children_per_cell; ++i)
    this->prolongation[i].reinit (this->dofs_per_cell,
				  this->dofs_per_cell);
  FETools::compute_embedding_matrices (*this, this->prolongation);
				   // Fill restriction matrices with L2-projection
  for (unsigned int i=0; i<GeometryInfo<dim>::children_per_cell; ++i)
    this->restriction[i].reinit (this->dofs_per_cell,
				  this->dofs_per_cell);
  FETools::compute_projection_matrices (*this, this->restriction);
  
				   // Compute support points, whivh
				   // are the tensor product of the
				   // Lagrange interpolation points in
				   // the constructor.
  Quadrature<dim> support_quadrature(points);
  this->unit_support_points = support_quadrature.get_points();
}

template <int dim>
std::string
FE_DGQ<dim>::get_name () const
{
				   // note that the
				   // FETools::get_fe_from_name
				   // function depends on the
				   // particular format of the string
				   // this function returns, so they
				   // have to be kept in synch

  std::ostringstream namebuf;  
  namebuf << "FE_DGQ<" << dim << ">(" << this->degree << ")";

  return namebuf.str();
}



template <int dim>
FiniteElement<dim> *
FE_DGQ<dim>::clone() const
{
				   // TODO[Prill] : There must be a better way
				   // to extract 1D quadrature points from the
				   // tensor product FE.
  
				   // Construct a dummy quadrature formula
				   // containing the FE's nodes:
  std::vector<Point<1> > qpoints(this->degree+1);
  for (unsigned int i=0; i<=this->degree; ++i)
    qpoints[i] = Point<1>(this->unit_support_points[i][0]);
  Quadrature<1> pquadrature(qpoints);
  
  return new FE_DGQ<dim>(pquadrature);
}


//---------------------------------------------------------------------------
// Auxiliary functions
//---------------------------------------------------------------------------


template <int dim>
std::vector<unsigned int>
FE_DGQ<dim>::get_dpo_vector (const unsigned int deg)
{
  std::vector<unsigned int> dpo(dim+1, 0U);
  dpo[dim] = deg+1;
  for (unsigned int i=1;i<dim;++i)
    dpo[dim] *= deg+1;
  return dpo;
}



template <int dim>
void
FE_DGQ<dim>::rotate_indices (std::vector<unsigned int> &numbers,
			     const char                 direction) const
{
  const unsigned int n = this->degree+1;
  unsigned int s = n;
  for (unsigned int i=1;i<dim;++i)
    s *= n;
  numbers.resize (s);
  
  unsigned int l = 0;

  if (dim==1)
    {
				       // Mirror around midpoint
      for (unsigned int i=n;i>0;)
	numbers[l++]=--i;
    }
  else
    {
      switch (direction)
	{
					   // Rotate xy-plane
					   // counter-clockwise
	  case 'z':
	    for (unsigned int iz=0;iz<((dim>2) ? n:1);++iz)
	      for (unsigned int j=0;j<n;++j)
		for (unsigned int i=0;i<n;++i)
		  {
		    unsigned int k = n*i-j+n-1 + n*n*iz;
		    numbers[l++] = k;
		  }
	    break;
					     // Rotate xy-plane
					     // clockwise
	  case 'Z':
	    for (unsigned int iz=0;iz<((dim>2) ? n:1);++iz)
	      for (unsigned int iy=0;iy<n;++iy)
		for (unsigned int ix=0;ix<n;++ix)
		  {
		    unsigned int k = n*ix-iy+n-1 + n*n*iz;
		    numbers[k] = l++;
		  }
	    break;
					     // Rotate yz-plane
					     // counter-clockwise
	  case 'x':
	    Assert (dim>2, ExcDimensionMismatch (dim,3));
	    for (unsigned int iz=0;iz<n;++iz)
	      for (unsigned int iy=0;iy<n;++iy)
		for (unsigned int ix=0;ix<n;++ix)
		  {
		    unsigned int k = n*(n*iy-iz+n-1) + ix;
		    numbers[l++] = k;
		  }
	    break;
					     // Rotate yz-plane
					     // clockwise
	  case 'X':
	    Assert (dim>2, ExcDimensionMismatch (dim,3));
	    for (unsigned int iz=0;iz<n;++iz)
	      for (unsigned int iy=0;iy<n;++iy)
		for (unsigned int ix=0;ix<n;++ix)
		  {
		    unsigned int k = n*(n*iy-iz+n-1) + ix;
		    numbers[k] = l++;
		  }
	    break;
	  default:
	    Assert (false, ExcNotImplemented ());
	}
    }
}



template <int dim>
void
FE_DGQ<dim>::
get_interpolation_matrix (const FiniteElement<dim> &x_source_fe,
			  FullMatrix<double>           &interpolation_matrix) const
{
				   // this is only implemented, if the
				   // source FE is also a
				   // DGQ element
  AssertThrow ((x_source_fe.get_name().find ("FE_DGQ<") == 0)
               ||
               (dynamic_cast<const FE_DGQ<dim>*>(&x_source_fe) != 0),
               typename FiniteElement<dim>::
               ExcInterpolationNotImplemented());
  
				   // ok, source is a Q element, so
				   // we will be able to do the work
  const FE_DGQ<dim> &source_fe
    = dynamic_cast<const FE_DGQ<dim>&>(x_source_fe);

  Assert (interpolation_matrix.m() == this->dofs_per_cell,
	  ExcDimensionMismatch (interpolation_matrix.m(),
				this->dofs_per_cell));
  Assert (interpolation_matrix.n() == source_fe.dofs_per_cell,
	  ExcDimensionMismatch (interpolation_matrix.n(),
				source_fe.dofs_per_cell));
  
  
				   // compute the interpolation
				   // matrices in much the same way as
				   // we do for the embedding matrices
				   // from mother to child.
  FullMatrix<double> cell_interpolation (this->dofs_per_cell,
					 this->dofs_per_cell);
  FullMatrix<double> source_interpolation (this->dofs_per_cell,
					   source_fe.dofs_per_cell);
  FullMatrix<double> tmp (this->dofs_per_cell,
			  source_fe.dofs_per_cell);
  for (unsigned int j=0; j<this->dofs_per_cell; ++j)
    {
                                       // generate a point on this
                                       // cell and evaluate the
                                       // shape functions there
      const Point<dim> p = generate_unit_point (j, this->dofs_per_cell,
                                                dealii::internal::int2type<dim>());
      for (unsigned int i=0; i<this->dofs_per_cell; ++i)
        cell_interpolation(j,i)
          = this->poly_space.compute_value (i, p);

      for (unsigned int i=0; i<source_fe.dofs_per_cell; ++i)
        source_interpolation(j,i)
          = source_fe.poly_space.compute_value (i, p);
    }

                                   // then compute the
                                   // interpolation matrix matrix
                                   // for this coordinate
                                   // direction
  cell_interpolation.gauss_jordan ();
  cell_interpolation.mmult (interpolation_matrix,
                            source_interpolation);

                                   // cut off very small values
  for (unsigned int i=0; i<this->dofs_per_cell; ++i)
    for (unsigned int j=0; j<source_fe.dofs_per_cell; ++j)
      if (std::fabs(interpolation_matrix(i,j)) < 1e-15)
        interpolation_matrix(i,j) = 0.;

				   // make sure that the row sum of
				   // each of the matrices is 1 at
				   // this point. this must be so
				   // since the shape functions sum up
				   // to 1
  for (unsigned int i=0; i<this->dofs_per_cell; ++i)
    {
      double sum = 0.;
      for (unsigned int j=0; j<source_fe.dofs_per_cell; ++j)
        sum += interpolation_matrix(i,j);

      Assert (std::fabs(sum-1) < 5e-14*std::max(this->degree,1U)*dim,
              ExcInternalError());
    }
}



template <int dim>
void
FE_DGQ<dim>::
get_face_interpolation_matrix (const FiniteElement<dim> &x_source_fe,
			       FullMatrix<double>       &interpolation_matrix) const
{
				   // this is only implemented, if the source
				   // FE is also a DGQ element. in that case,
				   // both elements have no dofs on their
				   // faces and the face interpolation matrix
				   // is necessarily empty -- i.e. there isn't
				   // much we need to do here.
  AssertThrow ((x_source_fe.get_name().find ("FE_DGQ<") == 0)
               ||
               (dynamic_cast<const FE_DGQ<dim>*>(&x_source_fe) != 0),
               typename FiniteElement<dim>::
               ExcInterpolationNotImplemented());
  
  Assert (interpolation_matrix.m() == 0,
	  ExcDimensionMismatch (interpolation_matrix.m(),
				0));
  Assert (interpolation_matrix.n() == 0,
	  ExcDimensionMismatch (interpolation_matrix.m(),
				0));
}



template <int dim>
void
FE_DGQ<dim>::
get_subface_interpolation_matrix (const FiniteElement<dim> &x_source_fe,
				  const unsigned int ,
				  FullMatrix<double>           &interpolation_matrix) const
{
				   // this is only implemented, if the source
				   // FE is also a DGQ element. in that case,
				   // both elements have no dofs on their
				   // faces and the face interpolation matrix
				   // is necessarily empty -- i.e. there isn't
				   // much we need to do here.
  AssertThrow ((x_source_fe.get_name().find ("FE_DGQ<") == 0)
               ||
               (dynamic_cast<const FE_DGQ<dim>*>(&x_source_fe) != 0),
               typename FiniteElement<dim>::
               ExcInterpolationNotImplemented());
  
  Assert (interpolation_matrix.m() == 0,
	  ExcDimensionMismatch (interpolation_matrix.m(),
				0));
  Assert (interpolation_matrix.n() == 0,
	  ExcDimensionMismatch (interpolation_matrix.m(),
				0));
}



template <int dim>
bool
FE_DGQ<dim>::hp_constraints_are_implemented () const
{
  return true;
}



template <int dim>
std::vector<std::pair<unsigned int, unsigned int> >
FE_DGQ<dim>::
hp_vertex_dof_identities (const FiniteElement<dim> &fe_other) const
{
				   // there are no such constraints for DGQ
				   // elements at all
  if (dynamic_cast<const FE_DGQ<dim>*>(&fe_other) != 0)
    return
      std::vector<std::pair<unsigned int, unsigned int> > ();
  else
    {
      Assert (false, ExcNotImplemented());
      return std::vector<std::pair<unsigned int, unsigned int> > ();
    }
}



template <int dim>
std::vector<std::pair<unsigned int, unsigned int> >
FE_DGQ<dim>::
hp_line_dof_identities (const FiniteElement<dim> &fe_other) const
{
				   // there are no such constraints for DGQ
				   // elements at all
  if (dynamic_cast<const FE_DGQ<dim>*>(&fe_other) != 0)
    return
      std::vector<std::pair<unsigned int, unsigned int> > ();
  else
    {
      Assert (false, ExcNotImplemented());
      return std::vector<std::pair<unsigned int, unsigned int> > ();
    }
}



template <int dim>
std::vector<std::pair<unsigned int, unsigned int> >
FE_DGQ<dim>::
hp_quad_dof_identities (const FiniteElement<dim>        &fe_other) const
{
				   // there are no such constraints for DGQ
				   // elements at all
  if (dynamic_cast<const FE_DGQ<dim>*>(&fe_other) != 0)
    return
      std::vector<std::pair<unsigned int, unsigned int> > ();
  else
    {
      Assert (false, ExcNotImplemented());
      return std::vector<std::pair<unsigned int, unsigned int> > ();
    }
}



template <int dim>
FiniteElementDomination::Domination
FE_DGQ<dim>::compare_for_face_domination (const FiniteElement<dim> &fe_other) const
{
				   // check whether both are discontinuous
				   // elements and both could dominate, see
				   // the description of
				   // FiniteElementDomination::Domination
  if (dynamic_cast<const FE_DGQ<dim>*>(&fe_other) != 0)
    return FiniteElementDomination::either_element_can_dominate;

  Assert (false, ExcNotImplemented());
  return FiniteElementDomination::neither_element_dominates;
}



template <int dim>
bool
FE_DGQ<dim>::has_support_on_face (const unsigned int shape_index,
				  const unsigned int face_index) const
{
  Assert (shape_index < this->dofs_per_cell,
	  ExcIndexRange (shape_index, 0, this->dofs_per_cell));
  Assert (face_index < GeometryInfo<dim>::faces_per_cell,
	  ExcIndexRange (face_index, 0, GeometryInfo<dim>::faces_per_cell));

  unsigned int n = this->degree+1;
  unsigned int n2 = n*n;
  
  switch (dim)
    {
      case 1:
    {
				       // in 1d, things are simple. since
				       // there is only one degree of
				       // freedom per vertex in this
				       // class, the first is on vertex 0
				       // (==face 0 in some sense), the
				       // second on face 1:
      return (((shape_index == 0) && (face_index == 0)) ||
	      ((shape_index == 1) && (face_index == 1)));
    };
      
      case 2:
    {
      if (face_index==0 && (shape_index % n) == 0)
	return true;
      if (face_index==1 && (shape_index % n) == this->degree)
	return true;
      if (face_index==2 && shape_index < n)
	return true;
      if (face_index==3 && shape_index >= this->dofs_per_cell-n)
	return true;
      return false;
    };
      
      case 3:
    {
      const unsigned int in2 = shape_index % n2;
	
				       // x=0
      if (face_index==0 && (shape_index % n) == 0)
	return true;
				       // x=1
      if (face_index==1 && (shape_index % n) == n-1)
	return true;
				       // y=0
      if (face_index==2 && in2 < n )
	return true;
				       // y=1
      if (face_index==3 && in2 >= n2-n)
	return true;
				       // z=0
      if (face_index==4 && shape_index < n2)
	return true;
				       // z=1
      if (face_index==5 && shape_index >= this->dofs_per_cell - n2)
	return true;
      return false;
    };

      default:
	Assert (false, ExcNotImplemented());
    }
  return true;
}



template <int dim>
unsigned int
FE_DGQ<dim>::memory_consumption () const
{
  Assert (false, ExcNotImplemented ());
  return 0;
}



template class FE_DGQ<deal_II_dimension>;

DEAL_II_NAMESPACE_CLOSE
