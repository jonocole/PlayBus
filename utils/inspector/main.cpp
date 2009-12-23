#include <QApplication>
#include <iostream>
#include <QVBoxLayout>
#include "../../bindings/qt-c++/PlayBus.h"
#include "Widgets.h"
#include <QTextEdit>
#include <QPushButton>
#include <QSignalMapper>

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    std::cout << "Connecting to playbus" << std::endl;    

    PlayBus m_pb( "PlaybusTest" );
    m_pb.setQueryMessages( true );
    m_pb.board();

    std::string s;

    QWidget w;
    QVBoxLayout l( &w );
    LineEdit input;
    TextEdit textedit;
    QPushButton clearButton( "Clear" );
    textedit.setReadOnly( true );
    w.layout()->addWidget( &input);
    w.layout()->addWidget( &textedit );
    w.layout()->addWidget( &clearButton );
    w.setWindowTitle( argv[1] );
    w.show();
    QObject::connect( &clearButton, SIGNAL( clicked()), &textedit, SLOT( clear()));
    QObject::connect( &m_pb, SIGNAL( message( QByteArray )), &textedit, SLOT( append( QByteArray)));
    QObject::connect( &input, SIGNAL( returnPressed( QByteArray )), &m_pb, SLOT( sendQuery( QByteArray )));
    QObject::connect( &input, SIGNAL( returnPressed()), &input, SLOT( clear()));

    app.exec();
}
