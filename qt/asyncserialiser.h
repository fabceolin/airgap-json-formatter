/**
 * @file asyncserialiser.h
 * @brief Centralized task queue that serializes Asyncify-dependent operations
 *
 * The AsyncSerialiser ensures that only one async task executes at a time,
 * preventing concurrent Asyncify suspensions that could crash the WASM runtime.
 */
#pragma once

#include <QObject>
#include <QQueue>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QTimer>
#include <QVariant>
#include <functional>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/**
 * @class AsyncSerialiser
 * @brief Singleton class that serializes async task execution
 *
 * This class provides a FIFO queue for async operations. Only one task
 * runs at a time (guarded by m_isBusy), ensuring no concurrent Asyncify
 * suspensions occur in the WebAssembly environment.
 *
 * Usage example:
 * @code
 * AsyncSerialiser::instance().enqueue("loadHistory", []() {
 *     QPromise<QVariant> promise;
 *     auto future = promise.future();
 *     // ... async work ...
 *     promise.addResult(QVariant::fromValue(result));
 *     promise.finish();
 *     return future;
 * });
 * @endcode
 */
class AsyncSerialiser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int queueLength READ queueLength NOTIFY queueLengthChanged)

public:
    /**
     * @brief Type alias for async tasks
     *
     * An AsyncTask is a callable that returns a QFuture<QVariant>.
     * The task will be executed when it reaches the front of the queue.
     */
    using AsyncTask = std::function<QFuture<QVariant>()>;

    /**
     * @brief Get the singleton instance
     * @return Reference to the single AsyncSerialiser instance
     */
    static AsyncSerialiser& instance();

    /**
     * @brief Enqueue an async task for serialized execution
     * @param taskName Identifier for logging and signals
     * @param task Lambda or function returning QFuture<QVariant>
     *
     * The task will be executed when all previously enqueued tasks
     * have completed. If the queue is empty and no task is running,
     * the task starts immediately (on the next event loop tick).
     */
    void enqueue(const QString& taskName, AsyncTask task);

    /**
     * @brief Clear all pending tasks (emergency reset)
     *
     * Stops the current task (if any), cancels its future, clears the
     * queue, and resets the busy flag. Use this for error recovery.
     */
    void clearQueue();

    /**
     * @brief Get the number of pending tasks in the queue
     * @return Number of tasks waiting to execute (excludes current task)
     */
    int queueLength() const { return m_queue.size(); }

    /**
     * @brief Check if JSPI (JavaScript Promise Integration) is available
     * @return True if browser supports JSPI, false otherwise
     *
     * JSPI allows WebAssembly to suspend and resume with multiple concurrent
     * suspensions, eliminating Asyncify's single-flight limitation.
     * When JSPI is available, the AsyncSerialiser queue could potentially
     * be bypassed for direct task execution.
     *
     * This queries window.JSPI_AVAILABLE set by jspi-detect.js
     */
    static bool jspiAvailable();

signals:
    /**
     * @brief Emitted when a task begins execution
     * @param taskName The name passed to enqueue()
     */
    void taskStarted(const QString& taskName);

    /**
     * @brief Emitted when a task completes (success or failure)
     * @param taskName The name passed to enqueue()
     * @param success True if task completed normally, false if cancelled/timeout
     */
    void taskCompleted(const QString& taskName, bool success);

    /**
     * @brief Emitted when a task exceeds the watchdog timeout
     * @param taskName The name of the timed-out task
     */
    void taskTimedOut(const QString& taskName);

    /**
     * @brief Emitted when the queue length changes
     */
    void queueLengthChanged();

    /**
     * @brief Emitted when queue length exceeds warning threshold
     * @param length Current queue length
     */
    void queueLengthWarning(int length);

    /**
     * @brief Emitted when task is rejected due to queue being full
     * @param taskName The name of the rejected task
     */
    void taskRejected(const QString& taskName);

private:
    AsyncSerialiser();
    ~AsyncSerialiser();
    AsyncSerialiser(const AsyncSerialiser&) = delete;
    AsyncSerialiser& operator=(const AsyncSerialiser&) = delete;

    void processNext();
    void onWatchdogTimeout();
    void onTaskFinished();

    struct QueuedTask {
        QString name;
        AsyncTask task;
    };

    QQueue<QueuedTask> m_queue;
    bool m_isBusy = false;
    QString m_currentTaskName;
    QTimer m_watchdog;
    QFutureWatcher<QVariant>* m_watcher = nullptr;

#ifdef __EMSCRIPTEN__
    // Emscripten fallback timer handle for WASM reliability
    long m_emscriptenTimerId = 0;
    void startEmscriptenWatchdog();
    void stopEmscriptenWatchdog();
    static void emscriptenWatchdogCallback(void* userData);
#endif

    static constexpr int WATCHDOG_TIMEOUT_MS = 30000;
    static constexpr int QUEUE_LENGTH_WARNING_THRESHOLD = 10;
    static constexpr int MAX_QUEUE_SIZE = 100;
};
