#include <iostream>
#include <thread>

#include <queue>

// Struct to store a message
struct Message {
    double value;  // The value calculated by the producer
    std::thread::id threadId;  // The id of the producer thread
    bool isLast;  // Flag indicating if this is the last message
};

// Class to consume the messages produced by the producer threads
class Consumer {
public:
    // Function to consume the messages
    void consume() {
        std::unique_lock<std::mutex> lock(mtx);
        while(true) {
            // Wait for a message to be available
            condVar.wait(lock, [this]{ return !msgQueue.empty(); });

            // Consume the message
            auto msg = msgQueue.front();
            msgQueue.pop();

            // If it is the last message, print a "finished" message
            if(msg.isLast) {
                std::cout << msg.threadId << " finished.\n";
                lastMessageCount++;
                if(lastMessageCount == 2) break;  // Break if both producer threads have finished
            } else {
                // Else, print the message
                std::cout << msg.value << " :sent " << msg.threadId << "\n";
            }
        }
    }

    // Function to push a message to the queue
    void pushMessage(const Message& msg) {
        std::lock_guard<std::mutex> lock(mtx);
        msgQueue.push(msg);
        condVar.notify_one();
    }

private:
    std::mutex mtx;  // Mutex to protect the queue
    std::condition_variable condVar;  // Condition variable to notify the consumer when a message is available
    std::queue<Message> msgQueue;  // Queue to store the messages
    int lastMessageCount = 0;  // Count of the number of "last" messages received
};

// Class to produce messages to be consumed by the consumer thread
class Producer {
public:
    Producer(Consumer& consumer) : consumer(consumer) {}

    // Function to produce messages
    void produce() {
        std::thread::id threadId = std::this_thread::get_id();  // Get the id of the current thread
        double number = std::hash<std::thread::id>{}(threadId);  // Calculate the initial number based on the thread id

        // Produce messages until the number reaches 0
        while(number > 0) {
            Message msg = { number, threadId, false };  // Create the message
            consumer.pushMessage(msg);  // Send the message to the consumer
            number /= 10;  // Reduce the number by a factor of 10
        }

        // Send the last message
        Message msg = { 0, threadId, true };
        consumer.pushMessage(msg);
    }

private:
    Consumer& consumer;  // Reference to the consumer
};

int main() {
    Consumer consumer;  // Create a consumer

    Producer producer1(consumer), producer2(consumer);  // Create two producers

    // Create the consumer thread and the producer threads
    std::thread consumerThread(&Consumer::consume, &consumer);
    std::thread producerThread1(&Producer::produce, &producer1);
    std::thread producerThread2(&Producer::produce, &producer2);

    // Wait for all threads to finish
    producerThread1.join();
    producerThread2.join();
    consumerThread.join();

    return 0;
}
