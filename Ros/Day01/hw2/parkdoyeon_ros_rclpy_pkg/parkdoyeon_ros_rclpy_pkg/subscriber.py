import rclpy

from rclpy.node import Node

from std_msgs.msg import Int32
from std_msgs.msg import String
from std_msgs.msg import Float32
from std_msgs.msg import Bool


class SubscriberNode(Node):

    def __init__(self):
        super().__init__('py_subscriber')

        self.cpp_int_subscriber = self.create_subscription(
            Int32,
            'cpp_int_topic',
            self.cpp_int_callback,
            10
        )

        self.cpp_string_subscriber = self.create_subscription(
            String,
            'cpp_string_topic',
            self.cpp_string_callback,
            10
        )

        self.cpp_float_subscriber = self.create_subscription(
            Float32,
            'cpp_float_topic',
            self.cpp_float_callback,
            10
        )

        self.cpp_bool_subscriber = self.create_subscription(
            Bool,
            'cpp_bool_topic',
            self.cpp_bool_callback,
            10
        )

        self.py_int_subscriber = self.create_subscription(
            Int32,
            'py_int_topic',
            self.py_int_callback,
            10
        )

        self.py_string_subscriber = self.create_subscription(
            String,
            'py_string_topic',
            self.py_string_callback,
            10
        )

        self.py_float_subscriber = self.create_subscription(
            Float32,
            'py_float_topic',
            self.py_float_callback,
            10
        )

        self.py_bool_subscriber = self.create_subscription(
            Bool,
            'py_bool_topic',
            self.py_bool_callback,
            10
        )

    def cpp_int_callback(self, msg):
        self.get_logger().info(
            f'[C++ -> Python] int: {msg.data}'
        )

    def cpp_string_callback(self, msg):
        self.get_logger().info(
            f'[C++ -> Python] string: {msg.data}'
        )

    def cpp_float_callback(self, msg):
        self.get_logger().info(
            f'[C++ -> Python] float: {msg.data:.2f}'
        )

    def cpp_bool_callback(self, msg):
        self.get_logger().info(
            f'[C++ -> Python] bool: {msg.data}'
        )

    def py_int_callback(self, msg):
        self.get_logger().info(
            f'[Python -> Python] int: {msg.data}'
        )

    def py_string_callback(self, msg):
        self.get_logger().info(
            f'[Python -> Python] string: {msg.data}'
        )

    def py_float_callback(self, msg):
        self.get_logger().info(
            f'[Python -> Python] float: {msg.data:.2f}'
        )

    def py_bool_callback(self, msg):
        self.get_logger().info(
            f'[Python -> Python] bool: {msg.data}'
        )


def main(args=None):
    rclpy.init(args=args)

    node = SubscriberNode()

    rclpy.spin(node)

    node.destroy_node()

    rclpy.shutdown()


if __name__ == '__main__':
    main()