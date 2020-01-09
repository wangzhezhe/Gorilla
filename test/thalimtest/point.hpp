

struct inner{
    inner(){};
    int innerx=1;
    int innery=2;
    int innerz=3;
    ~inner(){};


};

class point {
    public:
        double x;
        double y;
        double z;
        inner m_i;


        point(double a=0.0, double b=0.0, double c=0.0)
            : x(a), y(b), z(c) {}

        template<typename A> friend void serialize(A& ar, point& p);

        double operator*(const point& p) const {
            return p.x * x + p.y * y + p.z * z;
        }
};


template<typename A>
void serialize(A& ar, inner& i) {
    ar & i.innerx;
    ar & i.innery;
    ar & i.innerz;
}

template<typename A>
void serialize(A& ar, point& p) {
    ar & p.x;
    ar & p.y;
    ar & p.z;
    ar & p.m_i;
}
