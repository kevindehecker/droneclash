// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef air_VectorMath_hpp
#define air_VectorMath_hpp

#include "common/common_utils/Utils.hpp"
#include "common_utils/RandomGenerator.hpp"
STRICT_MODE_OFF
//if not using unaligned types then disable vectorization to avoid alignment issues all over the places
//#define EIGEN_DONT_VECTORIZE
#include <Eigen/Dense>
STRICT_MODE_ON

namespace msr { namespace airlib {

template <class Vector3T, class QuaternionT, class RealT>
class VectorMathT {
public:
    //IMPORTANT: make sure fixed size vectorizable types have no alignment assumption
    //https://eigen.tuxfamily.org/dox/group__TopicUnalignedArrayAssert.html
	typedef Eigen::Matrix<float, 1, 1> Vector1f;
	typedef Eigen::Matrix<double, 1, 1> Vector1d;
	typedef Eigen::Matrix<float,2,1,Eigen::DontAlign> Vector2f;
	typedef Eigen::Matrix<double,4,1,Eigen::DontAlign> Vector2d;
    typedef Eigen::Vector3f Vector3f;
	typedef Eigen::Vector3d Vector3d;
	typedef Eigen::Array3f Array3f;
	typedef Eigen::Array3d Array3d;
    typedef Eigen::Quaternion<float,Eigen::DontAlign> Quaternionf;
    typedef Eigen::Quaternion<double,Eigen::DontAlign> Quaterniond;
	typedef Eigen::Matrix<double, 3, 3> Matrix3x3d;
    typedef Eigen::Matrix<float, 3, 3> Matrix3x3f;
    typedef Eigen::AngleAxisd AngleAxisd;
    typedef Eigen::AngleAxisf AngleAxisf;

    typedef common_utils::Utils Utils;
    //use different seeds for each component
    typedef common_utils::RandomGenerator<RealT, std::normal_distribution<RealT>, 1> RandomGeneratorGausianXT;
    typedef common_utils::RandomGenerator<RealT, std::normal_distribution<RealT>, 2> RandomGeneratorGausianYT;
    typedef common_utils::RandomGenerator<RealT, std::normal_distribution<RealT>, 3> RandomGeneratorGausianZT;
    typedef common_utils::RandomGenerator<RealT, std::uniform_real_distribution<RealT>, 1> RandomGeneratorXT;
    typedef common_utils::RandomGenerator<RealT, std::uniform_real_distribution<RealT>, 2> RandomGeneratorYT;
    typedef common_utils::RandomGenerator<RealT, std::uniform_real_distribution<RealT>, 3> RandomGeneratorZT;

    struct Pose {
		EIGEN_MAKE_ALIGNED_OPERATOR_NEW
		Vector3T position;
		QuaternionT orientation;

		Pose() 
		{}

        Pose(Vector3T position_val, QuaternionT orientation_val)
        {
            orientation = orientation_val;
            position = position_val;
        }

		friend Pose operator-(const Pose& lhs, const Pose& rhs)
		{
			return VectorMathT::subtract(lhs, rhs);
		}

        static Pose nanPose() {
            static const Pose nan_pose(VectorMathT::nanVector(), VectorMathT::nanQuaternion());
            return nan_pose;
        }
    };

    struct Transform {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        Vector3T translation;
        QuaternionT rotation;
    };

    class RandomVectorT {
    public:
        RandomVectorT()
        {}
        RandomVectorT(RealT min_val, RealT max_val)
            : rx_(min_val, max_val), ry_(min_val, max_val), rz_(min_val, max_val)
        {
        }
        RandomVectorT(const Vector3T& min_val, const Vector3T& max_val)
            : rx_(min_val.x(), max_val.x()), ry_(min_val.y(), max_val.y()), rz_(min_val.z(), max_val.z())
        {
        }

        void reset()
        {
            rx_.reset(); ry_.reset(); rz_.reset();
        }

        Vector3T next()
        {
            return Vector3T(rx_.next(), ry_.next(), rz_.next());
        }
    private:
        RandomGeneratorXT rx_;
        RandomGeneratorYT ry_;
        RandomGeneratorZT rz_;
    };

    class RandomVectorGaussianT {
    public:
        RandomVectorGaussianT()
        {}
        RandomVectorGaussianT(RealT mean, RealT stddev)
            : rx_(mean, stddev), ry_(mean, stddev), rz_(mean, stddev)
        {
        }
        RandomVectorGaussianT(const Vector3T& mean, const Vector3T& stddev)
            : rx_(mean.x(), stddev.x()), ry_(mean.y(), stddev.y()), rz_(mean.z(), stddev.z())
        {
        }

        void reset()
        {
            rx_.reset(); ry_.reset(); rz_.reset();
        }

        Vector3T next()
        {
            return Vector3T(rx_.next(), ry_.next(), rz_.next());
        }
    private:
        RandomGeneratorGausianXT rx_;
        RandomGeneratorGausianYT ry_;
        RandomGeneratorGausianZT rz_;
    };

public:
    static float magnitude(const Vector2f& v)
    {
        return v.norm();
    }
    
    static RealT magnitude(const Vector3T& v) 
    {
        return v.norm();
    }
    
    static Vector3T rotateVector(const Vector3T& v, const QuaternionT& q, bool assume_unit_quat)
    {
        assume_unit_quat; // stop warning: unused parameter.
        //More performant method is at http://gamedev.stackexchange.com/a/50545/20758
        //QuaternionT vq(0, v.x(), v.y(), v.z());
        //QuaternionT qi = assume_unit_quat ? q.conjugate() : q.inverse();
        //return (q * vq * qi).vec();

        return q._transformVector(v);
    }

    static Vector3T rotateVectorReverse(const Vector3T& v, const QuaternionT& q, bool assume_unit_quat)
    {
        //QuaternionT vq(0, v.x(), v.y(), v.z());
        //QuaternionT qi = assume_unit_quat ? q.conjugate() : q.inverse();
        //return (qi * vq * q).vec();

        if (!assume_unit_quat)
            return q.inverse()._transformVector(v);
        else
            return q.conjugate()._transformVector(v);
    }    

    static Vector3T transformToBodyFrame(const Vector3T& v_world, const QuaternionT& q, bool assume_unit_quat)
    {
        return rotateVectorReverse(v_world, q, assume_unit_quat);
    }

    static Vector3T transformToWorldFrame(const Vector3T& v_body, const QuaternionT& q, bool assume_unit_quat)
    {
        return rotateVector(v_body, q, assume_unit_quat);
    }

    static Vector3T transformToWorldFrame(const Vector3T& v_body, const Pose& pose, bool assume_unit_quat)
    {
        //translate
        Vector3T translated = v_body + pose.position;
        //rotate
        return transformToWorldFrame(translated, pose.orientation, assume_unit_quat);
    }

    static QuaternionT negate(const QuaternionT& q)
	{
		//from Gazebo implementation
		return QuaternionT(-q.w(), -q.x(), -q.y(), -q.z());
	}


    static Vector3T getRandomVectorFromGaussian(RealT stddev = 1, RealT mean = 0)
    {
        return Vector3T(
            Utils::getRandomFromGaussian(stddev, mean),
            Utils::getRandomFromGaussian(stddev, mean),
            Utils::getRandomFromGaussian(stddev, mean)
        );
    }

    static QuaternionT flipZAxis(const QuaternionT& q)
    {
        //quaternion formula comes from http://stackoverflow.com/a/40334755/207661
        return QuaternionT(q.w(), -q.x(), -q.y(), q.z());
    }

    static void toEulerianAngle(const QuaternionT& q
        , RealT& pitch, RealT& roll, RealT& yaw)
    {
        RealT ysqr = q.y() * q.y();
        RealT t0 = -2.0f * (ysqr + q.z() * q.z()) + 1.0f;
        RealT t1 = +2.0f * (q.x() * q.y() + q.w() * q.z());
        RealT t2 = -2.0f * (q.x() * q.z() - q.w() * q.y());
        RealT t3 = +2.0f * (q.y() * q.z() + q.w() * q.x());
        RealT t4 = -2.0f * (q.x() * q.x() + ysqr) + 1.0f;

        t2 = t2 > 1.0f ? 1.0f : t2;
        t2 = t2 < -1.0f ? -1.0f : t2;

        pitch = std::asin(t2);
        roll = std::atan2(t3, t4);
        yaw = std::atan2(t1, t0);
    }

    static Vector3T toAngularVelocity(const QuaternionT& start, const QuaternionT& end, RealT delta_sec)
    {
        RealT p_s, r_s, y_s;
        toEulerianAngle(start, p_s, r_s, y_s);

        RealT p_e, r_e, y_e;
        toEulerianAngle(end, p_e, r_e, y_e);

        RealT p_rate = (p_e - p_s) / delta_sec;
        RealT r_rate = (r_e - r_s) / delta_sec;
        RealT y_rate = (y_e - y_s) / delta_sec;

        //TODO: optimize below
        //Sec 1.3, https://ocw.mit.edu/courses/mechanical-engineering/2-154-maneuvering-and-control-of-surface-and-underwater-vehicles-13-49-fall-2004/lecture-notes/lec1.pdf
        RealT wx = r_rate       + 0                             - y_rate * sinf(p_e);
        RealT wy = 0            + p_rate * cosf(r_e)            + y_rate * sinf(r_e) * cosf(p_e);
        RealT wz = 0            - p_rate * sinf(r_e)            + y_rate * cosf(r_e) * cosf(p_e);

        return Vector3T(wx, wy, wz);
    }

	static Vector3T nanVector()
	{
        static const Vector3T val(std::numeric_limits<RealT>::quiet_NaN(), std::numeric_limits<RealT>::quiet_NaN(), std::numeric_limits<RealT>::quiet_NaN());
        return val;
	}

	static QuaternionT nanQuaternion()
	{
		return QuaternionT(std::numeric_limits<RealT>::quiet_NaN(), std::numeric_limits<RealT>::quiet_NaN(), 
			std::numeric_limits<RealT>::quiet_NaN(), std::numeric_limits<RealT>::quiet_NaN());
	}

    static bool hasNan(const Vector3T& v)
    {
        return std::isnan(v.x()) || std::isnan(v.y()) || std::isnan(v.z());
    }
    static bool hasNan(const QuaternionT& q)
    {
        return std::isnan(q.x()) || std::isnan(q.y()) || std::isnan(q.z()) || std::isnan(q.w());
    }

    static QuaternionT toQuaternion(RealT pitch, RealT roll, RealT yaw)
    {
        QuaternionT q;
        RealT t0 = std::cos(yaw * 0.5f);
        RealT t1 = std::sin(yaw * 0.5f);
        RealT t2 = std::cos(roll * 0.5f);
        RealT t3 = std::sin(roll * 0.5f);
        RealT t4 = std::cos(pitch * 0.5f);
        RealT t5 = std::sin(pitch * 0.5f);

        q.w() = t0 * t2 * t4 + t1 * t3 * t5;
        q.x() = t0 * t3 * t4 - t1 * t2 * t5;
        q.y() = t0 * t2 * t5 + t1 * t3 * t4;
        q.z() = t1 * t2 * t4 - t0 * t3 * t5;
        return q;
    }

	//from http://osrf-distributions.s3.amazonaws.com/gazebo/api/dev/Pose_8hh_source.html
	static Vector3T coordPositionSubtract(const Pose& lhs, const Pose& rhs)
	{
		QuaternionT tmp(0,
			lhs.position.x() - rhs.position.x(),
			lhs.position.y() - rhs.position.y(),
			lhs.position.z() - rhs.position.z()
		);

		tmp = rhs.orientation.inverse() * (tmp * rhs.orientation);

		return tmp.vec();
	}
	static QuaternionT coordOrientationSubtract(const QuaternionT& lhs, const QuaternionT& rhs)
	{
		QuaternionT result(rhs.inverse() * lhs);
		result.normalize();
		return result;
	}
	static Pose subtract(const Pose& lhs, const Pose& rhs)
	{
		return Pose(coordPositionSubtract(lhs, rhs), coordOrientationSubtract(lhs.orientation, rhs.orientation));
	}


    static std::string toString(const Vector3T& vect, const char* prefix = nullptr)
    {
        if (prefix)
            return Utils::stringf("%s[%f, %f, %f]", prefix, vect[0], vect[1], vect[2]);
        else
            return Utils::stringf("[%f, %f, %f]", vect[0], vect[1], vect[2]);
    }
    static std::string toString(const QuaternionT& quaternion, bool add_eularian = false) 
    {
        if (!add_eularian)
            return Utils::stringf("[%f, %f, %f, %f]", quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z());
        else {
            RealT pitch, roll, yaw;
            toEulerianAngle(quaternion, pitch, roll, yaw);
            return Utils::stringf("[%f, %f, %f, %f]-[%f, %f, %f]",
                quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z(), pitch, roll, yaw);
        }
    }    
    static std::string toString(const Vector2f& vect)
    {
        return Utils::stringf("[%f, %f]", vect[0], vect[1]);
    }

    static RealT getYaw(const QuaternionT& q)
    {
        return std::atan2(2.0f * (q.z() * q.w() + q.x() * q.y())
            , - 1.0f + 2.0f * (q.w() * q.w() + q.x() * q.x()));
    }

    static RealT getPitch(const QuaternionT& q) 
    {
        return std::asin(2.0f * (q.y() * q.w() - q.z() * q.x()));
    }

    static RealT getRoll(const QuaternionT& q)
    {
        return std::atan2(2.0f * (q.z() * q.y() + q.w() * q.x())
            , 1.0f - 2.0f * (q.x() * q.x() + q.y() * q.y()));
    }

    static RealT normalizeAngleDegrees(RealT angle)
    {
        angle = static_cast<RealT>(std::fmod(angle, 360));
        if (angle > 180)
            return angle - 360;
        else if (angle < -180)
            return angle + 360;
        else
            return angle;
    }

	/**
	* \brief Extracts the yaw part from a quaternion, using RPY / euler (z-y'-z'') angles.
	* RPY rotates about the fixed axes in the order x-y-z,
	* which is the same as euler angles in the order z-y'-x''.
	*/
	static RealT yawFromQuaternion(const QuaternionT& q) {
		return atan2(2.0 * (q.w() * q.z() + q.x() * q.y()),
			1.0 - 2.0 * (q.y() * q.y() + q.z() * q.z()));
	}

	static QuaternionT quaternionFromYaw(RealT yaw) {
		return QuaternionT(Eigen::AngleAxisd(yaw, Vector3T::UnitZ()));
	}
};
typedef VectorMathT<Eigen::Vector3d, Eigen::Quaternion<double,Eigen::DontAlign>, double> VectorMathd;
typedef VectorMathT<Eigen::Vector3f, Eigen::Quaternion<float,Eigen::DontAlign>, float> VectorMathf;


}} //namespace
#endif
