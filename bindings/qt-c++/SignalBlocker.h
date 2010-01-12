#ifndef SIGNAL_BLOCKER_H_
#define SIGNAL_BLOCKER_H_

#include <QTimer>
#include <QEventLoop>
/**
  * @brief Waits for a Qt signal to be emitted before
  *        proceeding with code execution.
  * 
  * @author Jono Cole<jono@last.fm>
  */
class SignalBlocker : public QEventLoop
{
Q_OBJECT
public:
    SignalBlocker( QObject* o, const char* signal, int timeout)
    : m_ret( true )
    {
        m_timer.setSingleShot( true );
        connect( o, signal, SLOT( onSignaled()));
        
        if( timeout > -1 ) {
            m_timer.setInterval( timeout );
            connect( &m_timer, SIGNAL(timeout()), this, SLOT( onTimeout()));
        }
    }
    
    /** returns true if the signal fired otherwise false if
        the timeout fired */
    bool start()
    {
        if( m_ret == false ) return false;
        m_timer.start();
        m_loop.exec();
        return m_ret;
    }

private slots:
    void onSignaled()
    {
        m_ret = true;
        m_timer.stop();
        m_loop.quit();
    }

    void onTimeout()
    {
        m_ret = false;
        m_loop.quit();
    }

private:
    QEventLoop m_loop;
    bool m_ret;
    QTimer m_timer;
};

#endif //SIGNAL_BLOCKER_H_
