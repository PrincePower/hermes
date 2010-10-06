#define H2D_REPORT_WARN
#define H2D_REPORT_INFO
#define H2D_REPORT_VERBOSE
#define H2D_REPORT_FILE "application.log"
#include "hermes2d.h"

using namespace RefinementSelectors;

// This test makes sure that example 19-newton-timedep-flame works correctly.

const int INIT_REF_NUM = 2;            // Number of initial uniform mesh refinements.
const int P_INIT = 1;                  // Initial polynomial degree.
const double TAU = 0.5;                // Time step.
const double T_FINAL = 60.0;           // Time interval length.
const double NEWTON_TOL = 1e-4;        // Stopping criterion for the Newton's method.
const int NEWTON_MAX_ITER = 50;        // Maximum allowed number of Newton iterations.
MatrixSolverType matrix_solver = SOLVER_UMFPACK;  // Possibilities: SOLVER_UMFPACK, SOLVER_PETSC,
                                                  // SOLVER_MUMPS, and more are coming.

// Problem constants.
const double Le    = 1.0;
const double alpha = 0.8;
const double beta  = 10.0;
const double kappa = 0.1;
const double x1    = 9.0;

// Boundary markers.
int bdy_left = 1;

// Boundary conditions.
BCType bc_types(int marker)
  { return (marker == bdy_left) ? BC_ESSENTIAL : BC_NATURAL; }

scalar essential_bc_values_t(int ess_bdy_marker, double x, double y)
  { return (ess_bdy_marker == bdy_left) ? 1.0 : 0; }

scalar essential_bc_values_c(int ess_bdy_marker, double x, double y)
  { return 0; }

// Initial conditions.
scalar temp_ic(double x, double y, scalar& dx, scalar& dy)
  { return (x <= x1) ? 1.0 : exp(x1 - x); }

scalar conc_ic(double x, double y, scalar& dx, scalar& dy)
  { return (x <= x1) ? 0.0 : 1.0 - exp(Le*(x1 - x)); }

// Weak forms, definition of reaction rate omega.
# include "forms.cpp"

int main(int argc, char* argv[])
{
  // Load the mesh.
  Mesh mesh;
  H2DReader mloader;
  mloader.load("domain.mesh", &mesh);

  // Initial mesh refinements.
  for(int i = 0; i < INIT_REF_NUM; i++) mesh.refine_all_elements();

  // Create H1 spaces with default shapesets.
  H1Space tspace(&mesh, bc_types, essential_bc_values_t, P_INIT);
  H1Space cspace(&mesh, bc_types, essential_bc_values_c, P_INIT);
  int ndof = Space::get_num_dofs(Tuple<Space *>(&tspace, &cspace));
  info("ndof = %d.", ndof);

  // Previous time level solutions.
  Solution t_prev_time_1, c_prev_time_1, t_prev_time_2, c_prev_time_2, 
           t_prev_newton, c_prev_newton;

  // And their initial exact setting.
  t_prev_time_1.set_exact(&mesh, temp_ic); c_prev_time_1.set_exact(&mesh, conc_ic);
  t_prev_time_2.set_exact(&mesh, temp_ic); c_prev_time_2.set_exact(&mesh, conc_ic);
  t_prev_newton.set_exact(&mesh, temp_ic); c_prev_newton.set_exact(&mesh, conc_ic);

  // Filters for the reaction rate omega and its derivatives.
  DXDYFilter omega(omega_fn, Tuple<MeshFunction*>(&t_prev_newton, &c_prev_newton));
  DXDYFilter omega_dt(omega_dt_fn, Tuple<MeshFunction*>(&t_prev_newton, &c_prev_newton));
  DXDYFilter omega_dc(omega_dc_fn, Tuple<MeshFunction*>(&t_prev_newton, &c_prev_newton));

  // Initialize the weak formulation.
  WeakForm wf(2);
  wf.add_matrix_form(0, 0, callback(newton_bilinear_form_0_0), HERMES_UNSYM, HERMES_ANY, &omega_dt);
  wf.add_matrix_form_surf(0, 0, callback(newton_bilinear_form_0_0_surf), 3);
  wf.add_matrix_form(0, 1, callback(newton_bilinear_form_0_1), HERMES_UNSYM, HERMES_ANY, &omega_dc);
  wf.add_matrix_form(1, 0, callback(newton_bilinear_form_1_0), HERMES_UNSYM, HERMES_ANY, &omega_dt);
  wf.add_matrix_form(1, 1, callback(newton_bilinear_form_1_1), HERMES_UNSYM, HERMES_ANY, &omega_dc);
  wf.add_vector_form(0, callback(newton_linear_form_0), HERMES_ANY, 
                     Tuple<MeshFunction*>(&t_prev_time_1, &t_prev_time_2, &omega));
  wf.add_vector_form_surf(0, callback(newton_linear_form_0_surf), 3);
  wf.add_vector_form(1, callback(newton_linear_form_1), HERMES_ANY, 
                     Tuple<MeshFunction*>(&c_prev_time_1, &c_prev_time_2, &omega));

  // Initialize the FE problem.
  bool is_linear = false;
  FeProblem fep(&wf, Tuple<Space *>(&tspace, &cspace), is_linear);

  // Set up the solver, matrix, and rhs according to the solver selection.
  SparseMatrix* matrix = create_matrix(matrix_solver);
  Vector* rhs = create_vector(matrix_solver);
  Solver* solver = create_linear_solver(matrix_solver, matrix, rhs);

  // Project the initial condition on the FE space to obtain initial
  // coefficient vector for the Newton's method.
  info("Projecting initial condition to obtain initial vector for the Newton's method.");
  scalar* coeff_vec = new scalar[ndof];
  project_global(Tuple<Space *>(&tspace, &cspace), Tuple<MeshFunction *>(&t_prev_newton, &c_prev_newton), coeff_vec, matrix_solver);

  // Initialize views.
  ScalarView rview("Reaction rate", new WinGeom(0, 0, 800, 230));

  // Time stepping loop:
  double current_time = 0.0; int ts = 1;
  do 
  {
    info("---- Time step %d, t = %g s.", ts, current_time);

    // Perform Newton's iteration.
    int it = 1;
    while (1)
    {
      fep.assemble(coeff_vec, matrix, rhs, false);
      
      // Multiply the residual vector with -1 since the matrix 
      // equation reads J(Y^n) \deltaY^{n+1} = -F(Y^n).
      for (int i = 0; i < ndof; i++) rhs->set(i, -rhs->get(i));
      
      // Calculate the l2-norm of residual vector.
      double res_l2_norm = get_l2_norm(rhs);

      // Info for user.
      info("---- Newton iter %d, ndof %d, res. l2 norm %g", it, Space::get_num_dofs(Tuple<Space *>(&tspace, &cspace)), res_l2_norm);

      // If l2 norm of the residual vector is within tolerance, or the maximum number 
      // of iteration has been reached, then quit.
      if (res_l2_norm < NEWTON_TOL || it > NEWTON_MAX_ITER) break;

      // Solve the linear system and if successful, obtain the solutions.
      if(!solver->solve())
        error ("Matrix solver failed.\n");

        // Add \deltaY^{n+1} to Y^n.
      for (int i = 0; i < ndof; i++) coeff_vec[i] += solver->get_solution()[i];
      
      if (it >= NEWTON_MAX_ITER)
        error ("Newton method did not converge.");
     
      // Set current solutions to the latest Newton iterate and reinitialize filters of these solutions.
      Solution::vector_to_solutions(coeff_vec, Tuple<Space *>(&tspace, &cspace), Tuple<Solution *>(&t_prev_newton, &c_prev_newton));
      omega.reinit();
      omega_dt.reinit();
      omega_dc.reinit();

      it++;
    };

    // Visualization.
    DXDYFilter omega_view(omega_fn, Tuple<MeshFunction*>(&t_prev_newton, &c_prev_newton));
    rview.set_min_max_range(0.0,2.0);
    char title[100];
    sprintf(title, "Reaction rate, t = %g", current_time);
    rview.set_title(title);
    rview.show(&omega_view);

    // Update current time.
    current_time += TAU;

    // Store two time levels of previous solutions.
    t_prev_time_2.copy(&t_prev_time_1);
    c_prev_time_2.copy(&c_prev_time_1);
//    t_prev_time_1.set_coeff_vector(&tspace, coeff_vec);
//    c_prev_time_1.set_coeff_vector(&cspace, coeff_vec);
    Solution::vector_to_solutions(coeff_vec, Tuple<Space *>(&tspace, &cspace), Tuple<Solution *>(&t_prev_time_1, &c_prev_time_1));

    ts++;
  } 
  while (current_time <= T_FINAL);

  // Cleanup.
  delete [] coeff_vec;
  delete matrix;
  delete rhs;
  delete solver;

  info("Coordinate (  0,   8) value = %lf", t_prev_time_1.get_pt_value(0.0, 8.0));
  info("Coordinate (  8,   8) value = %lf", t_prev_time_1.get_pt_value(8.0, 8.0));
  info("Coordinate ( 15,   8) value = %lf", t_prev_time_1.get_pt_value(15.0, 8.0));
  info("Coordinate ( 24,   8) value = %lf", t_prev_time_1.get_pt_value(24.0, 8.0));
  info("Coordinate ( 30,   8) value = %lf", t_prev_time_1.get_pt_value(30.0, 8.0));
  info("Coordinate ( 40,   8) value = %lf", t_prev_time_1.get_pt_value(40.0, 8.0));
  info("Coordinate ( 50,   8) value = %lf", t_prev_time_1.get_pt_value(50.0, 8.0));
  info("Coordinate ( 60,   8) value = %lf", t_prev_time_1.get_pt_value(60.0, 8.0));

  info("Coordinate (  0,   8) value = %lf", c_prev_time_1.get_pt_value(0.0, 8.0));
  info("Coordinate (  8,   8) value = %lf", c_prev_time_1.get_pt_value(8.0, 8.0));
  info("Coordinate ( 15,   8) value = %lf", c_prev_time_1.get_pt_value(15.0, 8.0));
  info("Coordinate ( 24,   8) value = %lf", c_prev_time_1.get_pt_value(24.0, 8.0));
  info("Coordinate ( 30,   8) value = %lf", c_prev_time_1.get_pt_value(30.0, 8.0));
  info("Coordinate ( 40,   8) value = %lf", c_prev_time_1.get_pt_value(40.0, 8.0));
  info("Coordinate ( 50,   8) value = %lf", c_prev_time_1.get_pt_value(50.0, 8.0));
  info("Coordinate ( 60,   8) value = %lf", c_prev_time_1.get_pt_value(60.0, 8.0));

#define ERROR_SUCCESS                                0
#define ERROR_FAILURE                               -1
  double coor_x[8] = {0.0, 8.0, 15.0, 24.0, 30.0, 40.0, 50.0, 60.0};
  double coor_y = 8.0;
  double t_value[8] = {1.000000, 0.850946, 0.624183, 0.524876, 0.696210, 0.964166, 0.998641, 0.001120};
  double c_value[8] = {0.000000, -0.000000, 0.000002, 0.000009, 0.000001, -0.000000, 0.000042, 0.998844};
  for (int i = 0; i < 6; i++)
  {
    if (((t_value[i] - t_prev_time_1.get_pt_value(coor_x[i], coor_y)) < 1E-6) && ((c_value[i] - c_prev_time_1.get_pt_value(coor_x[i], coor_y)) < 1E-6))
    {
      printf("Success!\n");
    }
    else
    {
      printf("Failure!\n");
      return ERROR_FAILURE;
    }
  }
  return ERROR_SUCCESS;
}
