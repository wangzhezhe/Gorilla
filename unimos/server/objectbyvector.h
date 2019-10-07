template <typename T>
struct DataBlockByVec
{
    int blockID;
    std::vector<T> DataBlockValue;

    DataBlockByVec(std::vector<T> &dataArray)
    {
        DataBlockValue = std::move(dataArray);

        /*
        std::cout << "check the value at the dataBleock"<<std::endl;
        for(int i=0;i<10;i++){
            std::cout << "index " << i << " value "<<DataBlockValue[i]<<std::endl;
        }
        */
    }
    ~DataBlockByVec() {
        std::cout << "destructor of DataBlockByVec is called" <<std::endl;
        //delete the memroy allocated here
    }
};