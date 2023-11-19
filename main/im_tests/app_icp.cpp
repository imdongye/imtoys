#include "app_icp.h"
#include <stb_image.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <Eigen/Eigen>
#include <numeric>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace Eigen;

namespace
{
    void makeTransformedMesh(const Mesh& src, Mesh& dst, const glm::mat4& modelMat)
    {
        dst.tris = src.tris;
        dst.uvs = src.uvs;
        dst.deinitGL();
        for( const glm::vec3& p : src.poss ) {
            glm::vec4 hc = {p.x, p.y, p.z, 1};
            hc = modelMat*hc;
            dst.poss.push_back( glm::vec3(hc) );
        }
        for( const glm::vec3& n : src.nors ) {
            glm::vec4 hc = {n.x, n.y, n.z, 0};
            hc = modelMat*hc;
            dst.nors.push_back( glm::vec3(hc) );
        }
        dst.initGL();
    }

    struct Neighbor
    {
        std::vector<float> distances;
        std::vector<int> indices;
    };
    Neighbor nearest_neighbor(const MatrixXd &src, const MatrixXd &dst);

    Matrix4d icp_step(const MatrixXd &src, const MatrixXd &dst)
    {
        MatrixXd A = MatrixXd::Zero(6,6);
        MatrixXd H(4, 4);
        VectorXd x(6);
        VectorXd b(6);

        int row = src.rows();
        
        for( int i=0; i<row; i++ ) {
            MatrixXd tA(6, 6);
            Vector3d p, q, n, c;
            p = src.block<3, 1>(0, i);
            q = src.block<3, 1>(0, i);
            n = q-p;
            c = p.cross(n);
            tA << c.x()*c.x(), c.x()*c.y(), c.x()*c.z(), c.x()*n.x(), c.x()*n.y(), c.x()*n.z(),
                c.y()*c.x(), c.y()*c.y(), c.y()*c.z(), c.y()*n.x(), c.y()*n.y(), c.y()*n.z(),
                c.z()*c.x(), c.z()*c.y(), c.z()*c.z(), c.z()*n.x(), c.z()*n.y(), c.z()*n.z(),
                n.x()*c.x(), n.x()*c.y(), n.x()*c.z(), n.x()*n.x(), n.x()*n.y(), n.x()*n.z(),
                n.y()*c.x(), n.y()*c.y(), n.y()*c.z(), n.y()*n.x(), n.y()*n.y(), n.y()*n.z(),
                n.z()*c.x(), n.z()*c.y(), n.z()*c.z(), n.z()*n.x(), n.z()*n.y(), n.z()*n.z();
            A+=tA;
        }
        // A가 싱귤러 할수있나?
        JacobiSVD<MatrixXd> svd(A, ComputeThinU | ComputeThinV);
        // Pseudo inverse : V*invS*Ut
        MatrixXd A_inv = svd.matrixV()*svd.singularValues().asDiagonal().inverse() * svd.matrixU().transpose();
        x=A_inv*b;
        //x = svd.solve(b);

        H.setIdentity();
        H(1, 0) = x(2);
        H(0, 1) = -x(2);
        H(2, 0) = -x(1);
        H(0, 2) = x(1);
        H(2, 1) = x(0);
        H(1, 2) = -x(0);

        H(0, 3)=x(3);
        H(1, 3)=x(4);
        H(2, 3)=x(5);

        return H;
    }


    Matrix4d icp(const MatrixXd& src, const MatrixXd& dst, int max_iterations = 20, double tolerance = 0.001)
    {
        int row = src.rows();
        MatrixXd src4d = MatrixXd::Ones(3+1, row); // homogeneous src
        MatrixXd hrst = MatrixXd::Ones(3+1, row); // homogeneous moved pts
        MatrixXd tsrc = MatrixXd::Ones(3, row); // temp moved pts
        MatrixXd mdst = MatrixXd::Ones(3, row); // matched point with src

        Neighbor neighbor;
        Matrix4d TT, T;
        T.setIdentity();

        int iter = 0;

        for( int i = 0; i<row; i++ ) {
            src4d.block<3, 1>(0, i) = src.block<3, 1>(0, i);
            tsrc.block<3, 1>(0, i) = src.block<3, 1>(0, i);
        }

        double mean_error = 0;
        for( int i=0; i<max_iterations; i++ ) {
            neighbor = nearest_neighbor(tsrc, dst);

            for( int j=0; j<row; j++ ) {
                mdst.block<3, 1>(0, j) = dst.block<3, 1>(0, neighbor.indices[j]);
            }

            TT = icp_step(tsrc, mdst);
            T = TT*T;

            hrst = T*src4d;// compute shader로 병렬처리가능
            for( int j=0; j<row; j++ ) {
                tsrc.block<3, 1>(0, j) = hrst.block<3, 1>(0, j);
            }

            mean_error = std::accumulate(neighbor.distances.begin(), neighbor.distances.end(), 0.0)/neighbor.distances.size();
            printf("icp iter%d : %lf\n", i, mean_error);
        }

        return T;
    }

    float norm22(const Vector3d& pta, const Vector3d& ptb)
    {
        return (pta.x()-ptb.x())*(pta.x()-ptb.x())
            + (pta.y()-ptb.y())*(pta.y()-ptb.y())
            + (pta.z()-ptb.z())*(pta.z()-ptb.z());
    }
    // todo: kd tree
    Neighbor nearest_neighbor(const MatrixXd &src, const MatrixXd &dst)
    {
        int src_row = src.rows();
        int dst_row = dst.rows();
        Vector3d src_pt;
        Vector3d dst_pt;
        Neighbor neigh;
        float min = 100;
        int index = 0;
        float temp_dist = 0;

        for( int i=0; i < src_row; i++ ) {
            src_pt = src.block<3, 1>(0, i);
            min = 100;
            index = 0;
            temp_dist = 0;
            for( int j=0; j < dst_row; j++ ) {
                dst_pt = dst.block<3, 1>(0, j);
                temp_dist = norm22(src_pt, dst_pt);
                if( temp_dist < min ) {
                    min = temp_dist;
                    index = j;
                }
            }
            neigh.distances.push_back(min);
            neigh.indices.push_back(index);
        }

        return neigh;
    }
}

namespace lim
{
    AppICP::AppICP(): AppBase(1200, 780, APP_NAME)
    {
        prog.name = "icp";
        prog.home_dir = APP_DIR;
        prog.attatch("mvp_points.vs").attatch("blue.fs").link();

        camera.position = {0,0,5};
        camera.pivot = {0,0,0};
        camera.updateViewMat();

        Model model;
        model.importFromFile("assets/models/objs/woody.obj", true, false);
        log::pure(glm::to_string(model.position).c_str());
        log::pure("\n");
        log::pure(glm::to_string(model.scale).c_str());

        makeTransformedMesh(*model.my_meshes[0], dst, model.model_mat);

        glm::mat4 rigidMat = glm::translate(glm::vec3 {3,1, 0}) * glm::rotate(0.2f, glm::vec3 {0,0,1});
        makeTransformedMesh(dst, src, rigidMat);
    }
    AppICP::~AppICP()
    {
    }
    void AppICP::update()
    {
        glEnable(GL_PROGRAM_POINT_SIZE);
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        prog.use();

        prog.setUniform("view_Mat", camera.view_mat);
        prog.setUniform("proj_Mat", camera.proj_mat);
        prog.setUniform("camera_Pos", camera.position);

        prog.setUniform("modelMat", glm::mat4(1));
        glBindVertexArray(dst.vert_array);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dst.element_buf);
		glDrawElements(GL_POINTS, dst.tris.size()*3, GL_UNSIGNED_INT, 0);
		
        prog.setUniform("modelMat", icp_mat);
        glBindVertexArray(src.vert_array);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, src.element_buf);
		glDrawElements(GL_POINTS, src.tris.size()*3, GL_UNSIGNED_INT, 0);
    }
    void AppICP::renderImGui()
    {
        static bool isoverlay = true;
        log::drawViewer("logviewr##icp");
        //ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        ImGui::Begin("icp tester##icp", &isoverlay);
        if( ImGui::Button("iterate") ) {
            const int nrPoss = src.poss.size();
            // 열벡터로 나열
            Eigen::MatrixXd srcdata = Eigen::MatrixXd::Ones(3, nrPoss);
            Eigen::MatrixXd dstdata = Eigen::MatrixXd::Ones(3, nrPoss);

            for( int i=0; i<nrPoss; i++ ) {
                glm::vec3 p;
                p = src.poss[i];
                srcdata.block<3,1>(0, i) << p.x, p.y, p.z;

                p = dst.poss[i];
                dstdata.block<3,1>(0, i) << p.x, p.y, p.z;
            }

            Eigen::Matrix4d eMat = icp(srcdata, dstdata);

            for( int i=0; i<4; i++ ) for( int j=0; j<4; j++ ) {
                icp_mat[i][j] = eMat(j, i);
            }
            printf("done\n");
        }
        ImGui::End();
    }
}
