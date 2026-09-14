#include "robot_maze_game/gui_ros_bridge.h"
#include "robot_maze_game/mainwindow.h"

#include "rclcpp/rclcpp.hpp"

#include <QApplication>
#include <QByteArray>
#include <QPixmap>
#include <QString>
#include <QTimer>

#include <memory>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    rclcpp::init(argc, argv);
    QApplication application(argc, argv);

    auto bridge = std::make_shared<robot_maze_game::GuiRosBridge>();
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(bridge);

    robot_maze_game::MainWindow window(bridge);
    window.show();

    const QByteArray screenshotPath = qgetenv("ROBOT_MAZE_SCREENSHOT_PATH");
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(1500, &application, [&application, &window, screenshotPath] {
            window.grab().save(QString::fromLocal8Bit(screenshotPath), "PNG");
            application.quit();
        });
    }

    QTimer rosTimer;
    rosTimer.setInterval(10);
    QObject::connect(&rosTimer, &QTimer::timeout, [&application, &executor] {
        if (!rclcpp::ok()) {
            application.quit();
            return;
        }
        executor.spin_some();
    });
    rosTimer.start();

    const int result = application.exec();
    rosTimer.stop();
    executor.remove_node(bridge);
    bridge.reset();
    if (rclcpp::ok()) {
        rclcpp::shutdown();
    }
    return result;
}
