import torch

from torch import nn # 신경망을 만드는 도구 모음을 불러옴 
from torch.utils.data import DataLoader, random_split # 데이터 묶어서 전달, 데이터 무작위로 나누기 
from torchvision import datasets, transforms # 데이터셋 불러오기, 이미지를 신경망이 계산할 수 있는 텐서로 변환 

''' 검증 및 테스트 평가 '''

def evaluate(loader): # 가중치를 바꾸지 않고 데이터를 예측해서 평균 손실과 정확도를 구하는 함수
    model.eval() # 모델을 평가 모드로 설정하는 줄
    loss_sum, correct, count = 0.0, 0, 0 # 변수 초기화
    with torch.no_grad(): # 들여쓴 구간에서는 역전파용 연산 기록을 하지 않기때문에 기울기 계산을 하지 않음
        for images, labels in loader: # ''
            images, labels = images.to(device), labels.to(device) # ''
            logits = model(images) #''
            loss_sum += criterion(logits, labels).item() * len(labels) # 이번 묶음의 평균 손실 × 이미지 수를 누적
            correct += (logits.argmax(1) == labels).sum().item() # 맞힌 개수 세기
            count += len(labels) # 평가한 개수 누적
    return loss_sum / count, correct / count # 최종 결과 반환

''' 데이터 준비 '''

torch.manual_seed(0) # PyTorch가 만드는 가중치 난수의 시작 기준(seed)을 정함 

full = datasets.MNIST(
    "data", # 데이터를 저장하거나 찾을 폴더 
    train=True, # MNIST의 원본 학습 세트 60,000개 사용
    download=True, # 해당 폴더에 데이터가 없으면 다운로드
    transform=transforms.ToTensor() # 이미지를 꺼낼 때 텐서로 변환
) # MNIST 데이터를 불러옴 

train, val = random_split(
    full, # = datasets.MNIST
    [55000, 5000], # train 55000개, val 5000개
    generator=torch.Generator().manual_seed(0) # 데이터를 나눌 때 사용할 난수 생성기를 만들고 시드를 고정
) # 앞에서 가져온 full의 60,000개를 무작위로 나눈다.

test = datasets.MNIST(
    "data", # ''
    train=False, # MNIST의 원본 테스트 세트 10,000개 사용
    download=True, # ''
    transform=transforms.ToTensor() # ''
) # ''

train_loader = DataLoader(
    train, # 학습 이미지 55000개 
    batch_size=64, # 최대 64개씩 꺼내기 
    shuffle=True # 전체 데이터를 한 바퀴 돌 때마다 순서 섞기
) # 학습 데이터를 묶어서 꺼내주는 도구

val_loader = DataLoader(val, batch_size=256)
test_loader = DataLoader(test, batch_size=256)
# 각각 val과 test에서 최대 256개씩 이미지와 정답을 꺼내줌

''' 모델과 학습 설정 '''

device = torch.device(
    "cuda" if torch.cuda.is_available() else "cpu" 
) # CUDA를 사용할 수 있으면 "cuda", 아니면 "cpu"를 선택하라 

model = nn.Sequential( # 괄호 안에 적은 순서대로 데이터를 처리하는 신경망을 만듦
    nn.Flatten(), # 이미지 픽셀들을 한 줄로 펼침 
    nn.Linear(784, 128), # 입력값 784개를 받아 출력값 128개를 계산
    nn.ReLU(), # 앞에서 계산한 128개 값에 각각 적용 ReLU(z) = max(0, z)
    nn.Linear(128, 10) # 128개 값을 받아 숫자 0~9에 대한 점수 10개를 만듦
).to(device) # 모델의 가중치와 편향을 앞에서 선택한 장치로 옮긴다

criterion = nn.CrossEntropyLoss() # 예측과 정답을 비교할 손실 함수를 만든다 

optimizer = torch.optim.Adam(model.parameters(), lr=1e-3) # 모델의 가중치와 편향을 바꿔줄 옵티마이저를 만드는 줄로 학습률은 0.001로 설정 

epochs = 5 # 전체 데이터 학습을 5번 반복 (한 바퀴 안에서는 64개씩 묶어서 처리하고, 묶음마다 가중치를 갱신하므로 가중치를 5번만 갱신하는것이 아님)

'''미니배치 학습 루프 '''

for epoch in range(epochs): # epochs = 5
    model.train() # 모델을 학습 모드로 설정
    total_loss = 0.0 # 이번 epoch의 손실을 합산할 변수를 0으로 초기화

    for images, labels in train_loader: # 학습 데이터를 한 묶음씩 꺼내는 반복문
        images = images.to(device) # 이번 묶음의 이미지 텐서를 계산에 사용할 장치로 옮기는 줄
        labels = labels.to(device) # 정답 텐서도 같은 계산 장치로 옮기는 줄
        optimizer.zero_grad() # 이전 묶음에서 계산한 기울기 삭제 
        logits = model(images) # 현재 가중치로 숫자별 점수를 계산
        loss = criterion(logits, labels) # 예측 점수와 실제 정답을 비교해 이번 묶음의 평균 손실을 계산
        loss.backward() # 각 가중치와 편향에 대한 손실의 기울기를 계산해서 각각의 .grad에 저장
        optimizer.step() # 계산한 기울기를 이용해 Adam 규칙으로 가중치와 편향을 바꾼다 
        total_loss += loss.item() * images.size(0) # 결과 기록용
    # 학습 데이터를 64개씩 꺼내 예측하고, 손실을 계산한 뒤, 그 손실을 줄이도록 가중치를 바꾸는 반복문
    

    train_loss = total_loss / len(train)
    # 전체 손실 합계를 학습 이미지 수로 나눠 이번 epoch의 평균 손실을 구함

    val_loss, val_acc = evaluate(val_loader)

    print(
        f"Epoch {epoch + 1} | "
        f"학습 손실: {train_loss:.4f} | "
        f"검증 손실: {val_loss:.4f} | "
        f"검증 정확도: {val_acc:.2%}"
    )

test_loss, test_acc = evaluate(test_loader)

print(
    f"최종 테스트 손실: {test_loss:.4f} | "
    f"최종 테스트 정확도: {test_acc:.2%}"
)

import matplotlib.pyplot as plt

wrong_images = []
wrong_labels = []
wrong_predictions = []

model.eval()

with torch.no_grad():
    for images, labels in test_loader:
        images = images.to(device)
        labels = labels.to(device)

        logits = model(images)
        predictions = logits.argmax(1)

        # 예측과 정답이 다른 이미지의 위치 찾기
        wrong_indices = (predictions != labels).nonzero(as_tuple=True)[0]

        for index in wrong_indices:
            wrong_images.append(images[index].cpu())
            wrong_labels.append(labels[index].item())
            wrong_predictions.append(predictions[index].item())

            if len(wrong_images) == 9:
                break

        if len(wrong_images) == 9:
            break

# 틀린 이미지를 최대 9장까지 표시
fig, axes = plt.subplots(3, 3, figsize=(8, 8))

for i, ax in enumerate(axes.flat):
    ax.axis("off")

    if i < len(wrong_images):
        ax.imshow(wrong_images[i].squeeze(0), cmap="gray")
        ax.set_title(
            f"True: {wrong_labels[i]} / Pred: {wrong_predictions[i]}"
        )

plt.tight_layout()
plt.savefig("wrong_predictions.png", dpi=200)
plt.close(fig)

print(f"오분류 이미지 {len(wrong_images)}장을 wrong_predictions.png에 저장했습니다.")
