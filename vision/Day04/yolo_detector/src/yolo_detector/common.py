import math

from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, DurabilityPolicy


def image_qos():
    # 오래된 프레임을 쌓지 않고 최신 프레임을 우선합니다.
    return QoSProfile(history=HistoryPolicy.KEEP_LAST, depth=1,
                      reliability=ReliabilityPolicy.BEST_EFFORT,
                      durability=DurabilityPolicy.VOLATILE)


def positive(value, name):
    if not math.isfinite(value) or value <= 0:
        raise ValueError(f'{name} must be finite and > 0; received {value}')
    return value


def validate_classes(ids, names, model_names):
    if not ids or len(ids) != len(names):
        raise ValueError('class_ids and class_names must be nonempty and have equal lengths')
    if len(set(ids)) != len(ids):
        raise ValueError('class_ids contains duplicates')
    for class_id, name in zip(ids, names):
        expected = model_names.get(class_id)
        if expected != name:
            raise ValueError(f'class {class_id}: configured {name!r}, model name {expected!r}')
    return list(ids)
