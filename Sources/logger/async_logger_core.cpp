#include "logger/async_logger_core.h"
#include "logger/logfile.h"

namespace white {

const int AsyncLoggerCore::kBufferSize = 4000 * 1000;

AsyncLoggerCore::AsyncLoggerCore(const std::string &filename, int flush_interval) : 
flush_interval_(flush_interval),
is_stop_(true),
filename_(filename),
current_buffer_(std::make_shared<Buffer>(kBufferSize)),
next_buffer_(std::make_shared<Buffer>(kBufferSize))
{
    buffer_vector_.reserve(16);
}

AsyncLoggerCore::~AsyncLoggerCore()
{
    if(!is_stop_)
        Stop();
}

void AsyncLoggerCore::Append(const char *line, int len)
{
    std::lock_guard<std::mutex> locker(mutex_);
    if(current_buffer_->WritableBytes() > len)
        current_buffer_->Append(line, len);
    else
    {
        buffer_vector_.push_back(current_buffer_);
        if(next_buffer_)
            current_buffer_ = std::move(next_buffer_); // the next_buffer_ will be null after std::move
        else
            current_buffer_ = std::make_shared<Buffer>(kBufferSize); // if the next_buffer's space also ran out, create a new one.
        current_buffer_->Append(line, len);
        cond_.notify_all();
    }
}

void AsyncLoggerCore::ThreadFunction()
{
    LogFile output(filename_.c_str());
    std::shared_ptr<Buffer> new_buffer_1 = std::make_shared<Buffer>(kBufferSize); // the new buffer used to replace current buffer.
    std::shared_ptr<Buffer> new_buffer_2 = std::make_shared<Buffer>(kBufferSize);
    std::vector<std::shared_ptr<Buffer>> buffers_to_write;
    buffers_to_write.reserve(16);
    while(!is_stop_)
    {
        {
            std::unique_lock<std::mutex> locker(mutex_);
            if(buffer_vector_.empty())
                cond_.wait_for(locker, std::chrono::seconds(flush_interval_));
            buffer_vector_.push_back(current_buffer_); // put the current buffer into pending writing vector
            current_buffer_ = std::move(new_buffer_1); // use the new_buffer_1 to replace current buffer.
            buffers_to_write.swap(buffer_vector_); // new empty to write buffer vector.
            if(!next_buffer_) // if the second buffer was moved to the first buffer, give it a new buffer.
                next_buffer_ = std::move(new_buffer_2);
        }
        
        if(buffers_to_write.empty()) // if nothing to be write.
            continue;

        // if too much buffer in the vector, remove them. it may be too much data to write.
        if(buffers_to_write.size() > 25)
            buffers_to_write.erase(buffers_to_write.begin() + 2, buffers_to_write.end());
        
        // write into file
        for(auto &buffer : buffers_to_write)
            output.Append(buffer->ReadBeginConst(), buffer->ReadableBytes());

        if(buffers_to_write.size() > 2)
            buffers_to_write.resize(2);
        
        // replace the buffer already written to the file with the new buffer.
        if(!new_buffer_1)
        {
            new_buffer_1 = buffers_to_write.back();
            buffers_to_write.pop_back();
            new_buffer_1->Clear();
        }

        if(!new_buffer_2)
        {
            new_buffer_2 = buffers_to_write.back();
            buffers_to_write.pop_back();
            new_buffer_2->Clear();
        }

        buffers_to_write.clear();
        output.Flush();
    }
    output.Flush();
}

} // namespace white