// $Id: face_quad.cc,v 1.1.1.1 2008/03/19 06:41:21 gdiso Exp $

// The libMesh Finite Element Library.
// Copyright (C) 2002-2007  Benjamin S. Kirk, John W. Peterson

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// C++ includes

// Local includes
#include "face_quad.h"
#include "edge_edge2.h"





// ------------------------------------------------------------
// Quad class member functions
unsigned int Quad::key (const unsigned int s) const
{
  assert (s < this->n_sides());

  switch (s)
    {
    case 0:
      return  this->compute_key (this->node(0), this->node(1));

    case 1:
      return  this->compute_key (this->node(1), this->node(2));

    case 2:
      return  this->compute_key (this->node(2), this->node(3));

    case 3:
      return  this->compute_key (this->node(3), this->node(0));
    }


  // We will never get here...  Look at the code above.
  genius_error();
  return 0;
}



void Quad::nodes_on_side (const unsigned int i, std::vector<unsigned int> & nodes ) const
{
  switch (i)
  {
    case 0:
    {
      nodes.push_back(0);
      nodes.push_back(1);
      return;
    }
    case 1:
    {
      nodes.push_back(1);
      nodes.push_back(2);
      return;
    }
    case 2:
    {
      nodes.push_back(2);
      nodes.push_back(3);
      return;
    }
    case 3:
    {
      nodes.push_back(3);
      nodes.push_back(0);
      return;
    }
    default:
    {
      genius_error();
    }
  }
}


AutoPtr<DofObject> Quad::side (const unsigned int i) const
{
  assert (i < this->n_sides());

  Elem* edge = new Edge2;

  switch (i)
    {
    case 0:
      {
        edge->set_node(0) = this->get_node(0);
        edge->set_node(1) = this->get_node(1);

        AutoPtr<DofObject> ap_edge(edge);
        return ap_edge;
      }
    case 1:
      {
        edge->set_node(0) = this->get_node(1);
        edge->set_node(1) = this->get_node(2);

        AutoPtr<DofObject> ap_edge(edge);
        return ap_edge;
      }
    case 2:
      {
        edge->set_node(0) = this->get_node(2);
        edge->set_node(1) = this->get_node(3);

        AutoPtr<DofObject> ap_edge(edge);
        return ap_edge;
      }
    case 3:
      {
        edge->set_node(0) = this->get_node(3);
        edge->set_node(1) = this->get_node(0);

        AutoPtr<DofObject> ap_edge(edge);
        return ap_edge;
      }
    default:
      {
        genius_error();
      }
    }


  // We will never get here...  Look at the code above.
  genius_error();
  AutoPtr<DofObject> ap_edge(edge);
  return ap_edge;
}



bool Quad::is_child_on_side(const unsigned int c,
                             const unsigned int s) const
{
  assert (c < this->n_children());
  assert (s < this->n_sides());

  return (c == s || c == (s+1)%4);
}


Real Quad::quality (const ElemQuality q) const
{
  switch (q)
    {

      /**
       * Compue the min/max diagonal ratio.
       * This is modeled after the Hex element
       */
    case DISTORTION:
    case DIAGONAL:
    case STRETCH:
      {
    // Diagonal between node 0 and node 2
    const Real d02 = this->length(0,2);

    // Diagonal between node 1 and node 3
    const Real d13 = this->length(1,3);

    // Find the biggest and smallest diagonals
        if ( (d02 > 0.) && (d13 >0.) )
          if (d02 < d13) return d02 / d13;
          else return d13 / d02;
        else
          return 0.;
    break;
      }

    default:
      return Elem::quality(q);
    }

  /**
   * I don't know what to do for this metric.
   * Maybe the base class knows.  We won't get
   * here because of the default case above.
   */
  return Elem::quality(q);
}






std::pair<Real, Real> Quad::qual_bounds (const ElemQuality q) const
{
  std::pair<Real, Real> bounds;

  switch (q)
    {

    case ASPECT_RATIO:
      bounds.first  = 1.;
      bounds.second = 4.;
      break;

    case SKEW:
      bounds.first  = 0.;
      bounds.second = 0.5;
      break;

    case TAPER:
      bounds.first  = 0.;
      bounds.second = 0.7;
      break;

    case WARP:
      bounds.first  = 0.9;
      bounds.second = 1.;
      break;

    case STRETCH:
      bounds.first  = 0.25;
      bounds.second = 1.;
      break;

    case MIN_ANGLE:
      bounds.first  = 45.;
      bounds.second = 90.;
      break;

    case MAX_ANGLE:
      bounds.first  = 90.;
      bounds.second = 135.;
      break;

    case CONDITION:
      bounds.first  = 1.;
      bounds.second = 4.;
      break;

    case JACOBIAN:
      bounds.first  = 0.5;
      bounds.second = 1.;
      break;

    case SHEAR:
    case SHAPE:
    case CELL_SIZE:
      bounds.first  = 0.3;
      bounds.second = 1.;
      break;

    case DISTORTION:
      bounds.first  = 0.6;
      bounds.second = 1.;
      break;

    default:
      std::cout << "Warning: Invalid quality measure chosen." << std::endl;
      bounds.first  = -1;
      bounds.second = -1;
    }

  return bounds;
}




const unsigned short int Quad::_second_order_adjacent_vertices[4][2] =
{
  {0, 1}, // vertices adjacent to node 4
  {1, 2}, // vertices adjacent to node 5
  {2, 3}, // vertices adjacent to node 6
  {0, 3}  // vertices adjacent to node 7
};



const unsigned short int Quad::_second_order_vertex_child_number[9] =
{
  99,99,99,99, // Vertices
  0,1,2,0,     // Edges
  0            // Interior
};



const unsigned short int Quad::_second_order_vertex_child_index[9] =
{
  99,99,99,99, // Vertices
  1,2,3,3,     // Edges
  2            // Interior
};
