
#include <boost/serialization/serialization.hpp>
#include <boost/core/nvp.hpp>
#include <Eigen/Dense>
#include "entity.h"

namespace boost {
namespace serialization {

    /**
     * @brief Enables serialization of Eigen matrices using boost archives.
     */
    template<class Archive, class S, int Rows_, int Cols_, int Ops_, int MaxRows_, int MaxCols_>
    void serialize(Archive &ar,
        Eigen::Matrix<S, Rows_, Cols_, Ops_, MaxRows_, MaxCols_> &matrix,
        const unsigned int)
    {
        int rows = matrix.rows();
        int cols = matrix.cols();
        ar &make_nvp("rows", rows);
        ar &make_nvp("cols", cols);
        matrix.resize(rows, cols);// no-op if size does not change!

        // always save/load row-major
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) ar &make_nvp("val", matrix(r, c));
    }

    template<class Archive, class S, int Dim_, int Mode_, int Options_>
    void serialize(Archive &ar,
        Eigen::Transform<S, Dim_, Mode_, Options_> &transform,
        const unsigned int version)
    {
        serialize(ar, transform.matrix(), version);
    }

    /**
    * @brief Enables Serialization of Entities
    */
    template<typename Archive> void serialize(Archive &ar, benchmark::Entity &e, const unsigned int)
{
    ar &e.color_;
    ar &e.kind_;
    ar &e.pose_;
}

}// namespace serialization
}// namespace boost