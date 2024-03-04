#include"Buffer.h"

//private:
    // std::string buf_;    //用于存放数据

Buffer::Buffer(uint16_t sep): sep_(sep)
{

}

Buffer::~Buffer()
{
    
}

void Buffer::append(const char *data, size_t size)     //把数据追加到buf_中
{
    buf_.append(data, size);
}

void Buffer::appendwithsep(const char *data, size_t size)//把数据追加到buf_中, 附加报文头部
{
    if (data != nullptr && data[0] != '\0') {
        // printf("Buffer::appendwithhead data的地址: %p, data: %s\n", static_cast<const void*>(data), data);
    } else {
        printf("Buffer::appendwithhead data的地址: %p, data: (empty)\n", static_cast<const void*>(data));
    }

    if(sep_ == 0){
        buf_.append(data, size); // 没有分隔符, 直接添加
    }
    else if(sep_ == 1){
        buf_.append((char*)&size, 4);  //处理报文头
        buf_.append(data, size);   //添加报文本体
    }
    else if(sep_ == 2){
        buf_.append((char*)&size, 4);  //处理报文头
        buf_.append(data, size);   //添加报文本体
    }
    
}

// 从buf_的pos开始，删除nn个字节，pos从0开始。
void Buffer::erase(size_t pos,size_t nn)                             
{
    buf_.erase(pos,nn);
}

size_t Buffer::size()
{
    return buf_.size();
}
                                  //返回buf_的大小
const char* Buffer::data()
{
    return buf_.data(); // 将内容以字符数组形式返回 字符数组 char[];
}
                             //返回buf_的首地址
void Buffer::clear()
{
    buf_.clear();
}

bool Buffer::pickmessage(std::string &ss) //从buf中拆分出一个报文, 存放在ss中, 如果buf_中没有报文, 返回false
{
    if(buf_.size() == 0) return false;
    if(sep_ == 0) //没有分隔符
    {

    }
    else if(sep_ == 1)//四字节的报头
    {
        int len;
        memcpy(&len, buf_.data(), 4); //从buf_中获取报文头部
        //如果buf_中的数据量小于报文头部, 说明buf_中的报文内容不完整
        if(buf_.size() < len+4) return false;

        ss = buf_.substr(4, len);//从buf_中获取一个报文
        buf_.erase(0, len+4); //从buf_中删除刚才已获取的报文
    }
    else if(sep_ == 2) 
    {

    }

    return true;
}