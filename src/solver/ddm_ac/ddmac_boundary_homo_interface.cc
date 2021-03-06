/********************************************************************************/
/*     888888    888888888   88     888  88888   888      888    88888888       */
/*   8       8   8           8 8     8     8      8        8    8               */
/*  8            8           8  8    8     8      8        8    8               */
/*  8            888888888   8   8   8     8      8        8     8888888        */
/*  8      8888  8           8    8  8     8      8        8            8       */
/*   8       8   8           8     8 8     8      8        8            8       */
/*     888888    888888888  888     88   88888     88888888     88888888        */
/*                                                                              */
/*       A Three-Dimensional General Purpose Semiconductor Simulator.           */
/*                                                                              */
/*                                                                              */
/*  Copyright (C) 2007-2008                                                     */
/*  Cogenda Pte Ltd                                                             */
/*                                                                              */
/*  Please contact Cogenda Pte Ltd for license information                      */
/*                                                                              */
/*  Author: Gong Ding   gdiso@ustc.edu                                          */
/*                                                                              */
/********************************************************************************/



#include "simulation_system.h"
#include "semiconductor_region.h"
#include "boundary_condition_homo.h"
#include "petsc_utils.h"

void HomoInterfaceBC::DDMAC_Fill_Matrix_Vector( Mat A, Vec b, const Mat J, const PetscScalar omega, InsertMode & add_value_flag )
{

  BoundaryCondition::const_node_iterator node_it = nodes_begin();
  BoundaryCondition::const_node_iterator end_it = nodes_end();
  for(; node_it!=end_it; ++node_it )
  {

    // skip node not belongs to this processor
    if( (*node_it)->processor_id()!=Genius::processor_id() ) continue;

    std::vector<const SimulationRegion *> regions;
    std::vector<const FVM_Node *> fvm_nodes;

    // search all the fvm_node which has *node_it as root node, these nodes are the same in geometry,
    // but in different region.
    BoundaryCondition::region_node_iterator  rnode_it     = region_node_begin(*node_it);
    BoundaryCondition::region_node_iterator  end_rnode_it = region_node_end(*node_it);
    for(unsigned int i=0 ; rnode_it!=end_rnode_it; ++i, ++rnode_it  )
    {
      const SimulationRegion * region = (*rnode_it).second.first;
      const FVM_Node * fvm_node = (*rnode_it).second.second;
      if(!fvm_node->is_valid()) continue;

      regions.push_back( region );
      fvm_nodes.push_back( fvm_node );

      // the first semiconductor region
      if(i==0)
      {
        // load Jacobian entry of this node from J and fill into AC matrix A
        regions[0]->DDMAC_Fill_Nodal_Matrix_Vector(fvm_nodes[0], A, b, J, omega, add_value_flag);
      }

      // other region
      else
      {
        switch( region->type() )
        {
            case SemiconductorRegion :
            {
              //load Jacobian entry of this node from J and fill into AC matrix A with the location of fvm_node[0]
              regions[i]->DDMAC_Fill_Nodal_Matrix_Vector(fvm_nodes[i], A, b, J, omega, add_value_flag, regions[0], fvm_nodes[0]);

              // let indepedent variable of node in this region equal to indepedent variable of node in the first semiconductor region
              regions[i]->DDMAC_Force_equal(fvm_nodes[i], POTENTIAL, A, add_value_flag, regions[0], fvm_nodes[0]);
              regions[i]->DDMAC_Force_equal(fvm_nodes[i], ELECTRON, A, add_value_flag, regions[0], fvm_nodes[0]);
              regions[i]->DDMAC_Force_equal(fvm_nodes[i], HOLE, A, add_value_flag, regions[0], fvm_nodes[0]);
              if(regions[i]->get_advanced_model()->enable_Tl())
                regions[i]->DDMAC_Force_equal(fvm_nodes[i], TEMPERATURE, A, add_value_flag, regions[0], fvm_nodes[0]);

              // special process for energy balance equations
              unsigned int n_variables     = regions[i]->ebm_n_variables();
              unsigned int node_Tn_offset  = regions[i]->ebm_variable_offset(E_TEMP);
              unsigned int node_Tp_offset  = regions[i]->ebm_variable_offset(H_TEMP);

              //the indepedent variable number, we need 2 here.
              adtl::AutoDScalar::numdir=2;

              if(regions[i]->get_advanced_model()->enable_Tn())
              {
                AutoDScalar Tn = fvm_nodes[i]->node_data()->Tn(); Tn.setADValue(0,1.0);
                AutoDScalar Tn_semi = fvm_nodes[0]->node_data()->T();Tn_semi.setADValue(1,1.0);
                PetscInt Tn_offset_semi = fvm_nodes[0]->global_offset() + regions[0]->ebm_variable_offset(TEMPERATURE);
                if(regions[0]->get_advanced_model()->enable_Tn())
                {
                  AutoDScalar Tn_semi = fvm_nodes[0]->node_data()->Tn();
                  Tn_offset_semi = fvm_nodes[0]->global_offset() + regions[0]->ebm_variable_offset(E_TEMP);
                }

                AutoDScalar ffn = Tn - Tn_semi;

                // set Jacobian of governing equation ff1
                PetscInt real_row = fvm_nodes[i]->global_offset() + node_Tn_offset;
                PetscInt imag_row = fvm_nodes[i]->global_offset() + n_variables + node_Tn_offset;
                PetscInt real_col[2]={real_row, fvm_nodes[0]->global_offset() + Tn_offset_semi};
                PetscInt imag_col[2]={imag_row, fvm_nodes[0]->global_offset() + regions[0]->ebm_n_variables() + Tn_offset_semi};

                MatSetValues(A, 1, &real_row, 2, real_col, ffn.getADValue(), ADD_VALUES);
                MatSetValues(A, 1, &imag_row, 2, imag_col, ffn.getADValue(), ADD_VALUES);
              }

              if(regions[i]->get_advanced_model()->enable_Tp())
              {
                AutoDScalar Tp = fvm_nodes[i]->node_data()->Tp();     Tp.setADValue(0,1.0);
                AutoDScalar Tp_semi = fvm_nodes[0]->node_data()->T(); Tp_semi.setADValue(1,1.0);
                PetscInt Tp_offset_semi = fvm_nodes[0]->global_offset() + regions[0]->ebm_variable_offset(TEMPERATURE);
                if(regions[0]->get_advanced_model()->enable_Tp())
                {
                  Tp_semi = fvm_nodes[i]->node_data()->Tp();
                  Tp_offset_semi = fvm_nodes[0]->global_offset() + regions[0]->ebm_variable_offset(H_TEMP);
                }

                AutoDScalar ffp = Tp - Tp_semi;

                // set Jacobian of governing equation ff1
                PetscInt real_row = fvm_nodes[i]->global_offset() + node_Tp_offset;
                PetscInt imag_row = fvm_nodes[i]->global_offset() + n_variables + node_Tp_offset;
                PetscInt real_col[2]={real_row, fvm_nodes[0]->global_offset() + Tp_offset_semi};
                PetscInt imag_col[2]={imag_row, fvm_nodes[0]->global_offset() + regions[0]->ebm_n_variables() + Tp_offset_semi};

                MatSetValues(A, 1, &real_row, 2, real_col, ffp.getADValue(), ADD_VALUES);
                MatSetValues(A, 1, &imag_row, 2, imag_col, ffp.getADValue(), ADD_VALUES);
              }

              break;
            }

            case InsulatorRegion:
            {
              //load Jacobian entry of this node from J and fill into AC matrix A with the location of fvm_node[0]
              regions[i]->DDMAC_Fill_Nodal_Matrix_Vector(fvm_nodes[i], A, b, J, omega, add_value_flag, regions[0], fvm_nodes[0]);

              // let indepedent variable of node in this region equal to indepedent variable of node in the first semiconductor region
              regions[i]->DDMAC_Force_equal(fvm_nodes[i], POTENTIAL, A, add_value_flag, regions[0], fvm_nodes[0]);
              if(regions[i]->get_advanced_model()->enable_Tl())
                regions[i]->DDMAC_Force_equal(fvm_nodes[i], TEMPERATURE, A, add_value_flag, regions[0], fvm_nodes[0]);
              break;
            }
            default: genius_error();

        }
      }
    }
  }

  // the last operator is ADD_VALUES
  add_value_flag = ADD_VALUES;
}

