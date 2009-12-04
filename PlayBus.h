#ifndef PLAYBUS_H_
#define PLAYBUS_H_

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QList>
#include <QString>
#include <QtDebug>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QSignalMapper>
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
        m_timer.setInterval( timeout );
        connect( o, signal, SLOT( onSignaled()));
        connect( &m_timer, SIGNAL(timeout()), this, SLOT( onTimeout()));
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


/** @author Jono Cole <jono@last.fm>
  * @brief An interprocess message bus.
  *
  * This is a client side implemented bus, meaning that 
  * if no bus is running then a client will become the master node.
  * 
  * If the master node drops it's connection then another client node
  * will become the master and all other clients will connect to this.
  *
  * This bus is designed to be run on a single host using local servers.
  * the server mediation is handled at the kernel level by exploiting the 
  * logic that allows only one process to listen on a named pipe.
  *
  * This code will not work across distributed hosts as no master no
  * mediation is carried out. This could be extended by using a known
  * mediation algorithm.
  */
class PlayBus : public QObject
{
Q_OBJECT
public:
    /** @param name The name of the bus to connect to or create */
    PlayBus( const QString& name, QObject* parent = 0 )
    :QObject( parent ),
     m_busName( name )
    {
        connect( &m_server, SIGNAL( newConnection()),
                            SLOT( onIncomingConnection()));

        connect( this, SIGNAL( queryRequest( QString, QString )), SLOT( onQuery( QString, QString )));
    }

    ~PlayBus()
    {
        m_server.close();
    }

    void board()
    {
        reinit();
    }

    const QByteArray& lastMessage() const
    {
        return m_lastMessage;
    }


public slots:
 
    QString sendQuery( QString request, int timeout = 100 )
    {
        QString uuid = QUuid::createUuid();
        m_dispatchedQueries << uuid;
        static int test = 0;
        sendMessage( uuid + " " + request + " " + QString::number((test++)) );

        SignalBlocker blocker( this, SIGNAL( queryRequest(QString,QString)), timeout );
        
        while( blocker.start()) {
           if( m_lastQueryUuid == uuid ) {
                return m_lastQueryResponse;
            }
        }
        return QString();
    }

    void sendQueryResponse( QString uuid, QString message )
    {
        sendMessage( uuid + " " + message );
    }

   /** send the message around the bus */
    void sendMessage( const QString& msg )
    {
        if( m_server.isListening() && msg == "ROSTER" ) {
            processCommand( 0, "ROSTER" );
            return;
        }       

        foreach( QLocalSocket* socket, m_sockets ) {
            socket->write( (msg + "\n" ).toUtf8().data()  );
            socket->flush();
        }

    }

signals:
    /** a new message has been received from the bus */
    void message( const QString& msg );

    void queryRequest( const QString& uuid, const QString& message);

    void nodes( const QStringList& );

private slots:

    void onQuery( QString uuid, QString query )
    {
        sendQueryResponse( uuid, "Blah response: " + query );
    }

    void onSocketConnected()
    {
        QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
        addSocket( socket );
    }

    void reinit()
    {
        foreach( QLocalSocket* socket, m_sockets ) {
            m_sockets.removeAll(socket);
            socket->disconnect();
            socket->close();
            socket->deleteLater();
        }

        if( m_server.listen( m_busName )) {
            return;
        }

        m_server.close();
        QLocalSocket* socket = new QLocalSocket( this );
        connect( socket, SIGNAL( connected()), SLOT( onSocketConnected()));
        connect( socket, SIGNAL( disconnected()), SLOT( reinit()));
        connect( socket, SIGNAL( error(QLocalSocket::LocalSocketError)), SLOT( onError(QLocalSocket::LocalSocketError)));
        socket->connectToServer( m_busName );
    }

    void onError( const QLocalSocket::LocalSocketError& e )
    {
        if( e == QLocalSocket::ConnectionRefusedError )
            QFile::remove( QDir::tempPath() + "/" + m_busName );

        reinit();
    }

    void onIncomingConnection()
    {
        QLocalSocket* socket = 0;
        while( (socket = m_server.nextPendingConnection()) )
        {
            addSocket( socket );
        }
    }

    void processCommand( QLocalSocket* socket, const QByteArray& data )
    {
        m_lastMessage = data;
        QRegExp queryRE("^(\\{.{8}-.{4}-.{4}-.{4}-.{12}\\}) (.*)$");
        if( queryRE.indexIn( data ) > -1) {
            QString uuid = queryRE.cap(1);
            
            // ignore any queries that have already been seen
            if( !m_dispatchedQueries.contains( uuid ) && 
                 m_servicedQueries.contains( uuid )) return;

            m_lastQueryUuid = uuid;
            m_lastQueryResponse = queryRE.cap(2);
            m_servicedQueries << m_lastQueryUuid;
            emit queryRequest( m_lastQueryUuid, m_lastQueryResponse);
            return;
        }
        emit message( QString( data ) );
    }

    void onSocketData()
    {
        QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());

        while( socket->canReadLine()) {
            QByteArray data = socket->readLine();
            data.chop( 1 ); // remove trailing /n
            
            foreach( QLocalSocket* aSocket, m_sockets ) {
                if( aSocket == socket ) continue;
                aSocket->write( data + "\n" );
                aSocket->flush();
            }
            
            processCommand( socket, data );

        }
    }
    
    void onSocketDestroyed( QObject* o )
    {
        QLocalSocket* s = static_cast<QLocalSocket*>(o);
        m_sockets.removeAll( s );
    }

private:
    void addSocket( QLocalSocket* socket )
    {
        connect( socket, SIGNAL(readyRead()), SLOT(onSocketData()));
        connect( socket, SIGNAL(destroyed(QObject*)), SLOT(onSocketDestroyed(QObject*)));
        QSignalMapper* mapper = new QSignalMapper(socket);
        mapper->setMapping( socket, socket );
        connect( mapper, SIGNAL(mapped(QObject*)), SLOT(onSocketDestroyed(QObject*)));
        connect( socket, SIGNAL(disconnected()), mapper, SLOT( map()));
        m_sockets << socket;
    }

    const QStringList nodeList( const QString& data )
    {
        QString str( data );
        str = str.mid( 7 ); // remove NODES [
        str.chop( 1 ); // remove ]
        return str.split( "," );
    }

    QString m_busName;
    QLocalServer m_server;
    QList<QLocalSocket*> m_sockets;
    QByteArray m_lastMessage;
    QList<QString> m_dispatchedQueries;
    QList<QString> m_servicedQueries;
    QString m_lastQueryResponse;
    QString m_lastQueryUuid;
};


#endif //PLAYBUS_H_
