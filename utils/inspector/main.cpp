#include <QApplication>
#include <iostream>
#include <QVBoxLayout>
#include "PlayBus.h"
#include "LineEdit.h"
#include <QTextEdit>
#include <QPushButton>
#include <QSignalMapper>

int main( int argc, char* argv[] )
{
    QApplication app( argc, argv );

    std::cout << "Connecting to playbus" << std::endl;    

    PlayBus m_pb( "PlaybusTest" );
    m_pb.board();

    std::string s;

    QWidget w;
    QVBoxLayout l( &w );
    LineEdit input;
    QTextEdit textedit;
    QPushButton clearButton( "Clear" );
    textedit.setReadOnly( true );
    w.layout()->addWidget( &input);
    w.layout()->addWidget( &textedit );
    w.layout()->addWidget( &clearButton );
    w.setWindowTitle( argv[1] );
    w.show();
    QObject::connect( &clearButton, SIGNAL( clicked()), &textedit, SLOT( clear()));
    QObject::connect( &m_pb, SIGNAL( message( QString )), &textedit, SLOT( append( QString)));
    QObject::connect( &input, SIGNAL( returnPressed(QString)), &m_pb, SLOT( sendMessage( QString )));
    QObject::connect( &input, SIGNAL( returnPressed()), &input, SLOT( clear()));

    app.exec();
}
