/*!
 * @file Queue.h
 *
 * Queue is a linear data structure that follows a particular order in which the operations are performed for storing data.
 * The order is First In First Out (FIFO).
 * One can imagine a queue as a line of people waiting to receive something in sequential order which starts from the beginning
 * of the line. It is an ordered list in which insertions are done at one end which is known as the rear and deletions are done
 * from the other end known as the front.
 *
 * https://www.geeksforgeeks.org/array-implementation-of-queue-simple/
 *
 * Written by Roel, Rolenco Leusden
 *
 * BSD license, all text here must be included in any redistribution.
 * See the LICENSE file for details.
 *
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

// Uncomment to enable printing out nice debug messages.
//#define QUEUE_DEBUG

// Setup debug printing macros. Define where debug output will be printed to Serial.
#ifdef QUEUE_DEBUG
#define QDEBUG_PRINT(...)    \
    {                        \
        printf(__VA_ARGS__); \
    }
#else
#define QDEBUG_PRINT(...) \
    {                     \
    }
#endif

class   Queue
{
    
private:
    int front;      // First element in memory queue.
    int rear;       // rear Store index.
    int capacity;   // Array size.

    int *m_queue;     // ptr to queue[] array.

public:
    /*
        Constrctor
    */
    Queue(int _capacity)
    {
        front = -1;
        rear = -1;
        capacity = _capacity;
        m_queue = new int[capacity]; // or queue = (int*)malloc(capacity);    // create queue[] on heap memory.
    }

    /*
        Destructor
    */
    ~Queue()
    {
        delete[] m_queue; // remove queue[] from heap memory.
    }

    /*
        Enqueue: Addition of an element to the queue. Adding an element will be performed after checking whether the queue is full or not.
        If rear < n which indicates that the array is not full then store the element at arr[rear] and increment rear by 1 but
        if rear == n then it is said to be an Overflow condition as the array is full.

        function to insert an element at the rear of the queue
    */
    void Enqueue(int element)
    {
        // check queue is full or not
        if (rear == (capacity - 1))
        {
            QDEBUG_PRINT("\nEnqueue()::Queue is full\n");
        }
        else
        { // insert element at the rear index.
            rear++;
            m_queue[rear] = element;

            if (front == -1)
            {
                front = 0;
            }

            QDEBUG_PRINT("\nEnqueue(); index: %d = Element: %d", rear, m_queue[rear]);
        }

        return;
    }

    /*
        Dequeue: Removal of an element from the queue. An element can only be deleted when there is at least an element to delete i.e.
        rear > 0. Now, the element at arr[front] can be deleted but all the remaining elements have to shift to the left by one position
        in order for the dequeue operation to delete the second element from the left on another dequeue operation.

        function to load an front element from queue and delete an element from the front of the queue

        return INT32_MIN => empty
    */
    int Dequeue()
    {
        if ( IsEmpty())
        { // if queue[] is empty
            QDEBUG_PRINT("\nDequeue()::Queue is empty\n");
            return INT32_MIN;
        }
        else
        {
            int element = m_queue[front];

            // Delete element, shift all the elements from index 2 till rear to the left by one.
            for (int i = 0; i < rear - 1; i++)
            {
                m_queue[i] = m_queue[i + 1];
            }

            // store INT32_MIN at rear indicating there's no element.
            if (rear < capacity)
            {
                m_queue[rear] = INT32_MIN;
            }

            // decrement rear index.
            // rear--;
            if( --rear == -1)
            {
                front = -1;
            }

            QDEBUG_PRINT("\nDequeue(), index: %d = Element is: %d", front, element);

            return element;
        }
    }

    /*
        isEmpty(): Check if the queue[] is empty.

        return: true = empty.
    */
    bool IsEmpty()
    {
        if (front == -1 || rear == -1)
        {
            // Queue[] is empty.
            return true;
        }

        return false;
    }

    /*
        Peek: Get the front element from the queue
        i.e. queue[front] if the queue is not empty.

        return INT32_MIN => empty
    */
    int Peek()
    {
        if (IsEmpty())
        {
            QDEBUG_PRINT("\nPeek()::Queue is Empty\n");
            return INT32_MIN;
        }

        QDEBUG_PRINT("\nPeek Element is: %d", m_queue[front]);
        return m_queue[front];
    }

    // TODO:
    //      isFull(): This operation indicates whether the queue is full or not.
    //      Count(): This operation returns the count of the queue i.e. the total number of elements it contains.

};

#endif