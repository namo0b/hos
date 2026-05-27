// OWNER AREA: A - Frontend + Web Integration

// 화면 전체에서 공유하는 데이터 저장소
const state = {
    records: [], // 진료 기록 목록
    queue: [], // 진료 대기열
    selectedPatientName: "" // 진료 기록 화면에서 현재 선택한 환자 이름
};

const viewTitles = {
    registerView: "환자 접수",
    queueView: "진료 대기 순서",
    recordsView: "환자 진료 기록",
    priorityView: "우선순위 안내"
};

// HTML 요소를 JS 변수에 연결
const form = document.querySelector("#patientForm");
const recordTable = document.querySelector("#recordTable");
const patientList = document.querySelector("#patientList");
const recordCardHeader = document.querySelector("#recordCardHeader");
const queueTable = document.querySelector("#queueTable");
const searchInput = document.querySelector("#searchInput");
const refreshButton = document.querySelector("#refreshButton");
const completeButton = document.querySelector("#completeButton");
const resultBox = document.querySelector("#resultBox");
const completeBox = document.querySelector("#completeBox");
const statusBadge = document.querySelector("#serverStatus");
const viewTitle = document.querySelector("#viewTitle");

// 중요도 숫자를 CSS 클래스명으로 변경하는 함수
function priorityClass(priority) {
    if (priority <= 2) {
        return "high"; // 1, 2단계
    }
    if (priority === 3) {
        return "mid"; // 3단계
    }
    return ""; // 4, 5단계
}

function escapeHtml(value) { // HTML에 넣기 전에 특수 문자를 안전한 문자로 변경
    return String(value ?? "")
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#39;");
}

function setStatus(text, className = "ok") { // 상단 서버 상태 문구 변경
    statusBadge.textContent = text;
    statusBadge.className = `status ${className}`;
}

// 왼쪽 메뉴를 눌렀을 때 화면 변경
function switchView(viewId) {
    document.querySelectorAll(".view").forEach((view) => {
        view.classList.toggle("active", view.id === viewId);
    });

    document.querySelectorAll(".nav-button").forEach((button) => {
        button.classList.toggle("active", button.dataset.view === viewId);
    });

    viewTitle.textContent = viewTitles[viewId];
}

function renderSummary() { // 상단 요약 카드 숫자 3개 갱신(대기 환자/1-2단계 대기/전체 기록)
    document.querySelector("#queueCount").textContent = state.queue.length;
    document.querySelector("#urgentQueueCount").textContent = state.queue.filter((patient) => patient.finalPriority <= 2).length;
    document.querySelector("#totalCount").textContent = state.records.length;
}

// 진료 대기열 표를 그리는 함수
function renderQueue() {
    queueTable.innerHTML = "";
    completeButton.disabled = state.queue.length === 0;

    if (state.queue.length === 0) { // 대기 환자 없음
        const row = document.createElement("tr");
        row.innerHTML = `<td colspan="6">현재 진료 대기 환자가 없습니다.</td>`;
        queueTable.appendChild(row);
        return;
    }

    state.queue.forEach((patient, index) => { // 대기 환자 있으면 큐를 돌며 tr 만들고 큐 테이블 삽입
        const row = document.createElement("tr");
        row.className = index === 0 ? "next-row" : "";
        row.innerHTML = `
            <td>${index + 1}</td>
            <td><span class="priority-badge ${priorityClass(patient.finalPriority)}">${patient.finalPriority}단계</span></td>
            <td>${escapeHtml(patient.name)}</td>
            <td>${escapeHtml(patient.symptom)}</td>
            <td>${escapeHtml(patient.patientType)}</td>
            <td>${escapeHtml(patient.time)}</td>
        `;
        queueTable.appendChild(row);
    });
}

// 전체 진료 기록을 환자 이름별로 묶고 최근순으로 정리
function groupRecordsByPatient(records) {
    const groups = new Map();

    records.forEach((record) => {
        if (!groups.has(record.name)) {
            groups.set(record.name, []);
        }
        groups.get(record.name).push(record);
    });

    groups.forEach((items) => {
        items.sort((a, b) => String(b.time).localeCompare(String(a.time)));
    });

    return groups;
}

function getFilteredPatientGroups() { // 검색창 값 기준 환자 그룹 필터링 및 환자 이름 순 정렬
    const keyword = searchInput.value.trim().toLowerCase();
    const groups = groupRecordsByPatient(state.records);

    return [...groups.entries()]
        .filter(([name, records]) => { // 필터링
            if (keyword.length === 0) {
                return true;
            }

            const searchable = `${name} ${records.map((record) => record.symptom).join(" ")}`.toLowerCase();
            return searchable.includes(keyword);
        })
        .sort(([nameA], [nameB]) => nameA.localeCompare(nameB, "ko")); // 이름순 정렬
}

// 진료 기록 화면을 그리는 함수
// CHANGED: 백엔드 응답은 그대로 두고 프론트에서 환자 이름순 목록과 최근순 카드로 재구성
function renderRecords() { // 환자 목록과 선택한 환자의 기록 카드를 화면에 그리는 함수
    const patientGroups = getFilteredPatientGroups();

    patientList.innerHTML = "";
    recordTable.innerHTML = "";

    if (patientGroups.length === 0) { // 진료 기록 없을 때
        state.selectedPatientName = "";
        recordCardHeader.textContent = "표시할 진료 기록이 없습니다";
        patientList.innerHTML = `<div class="empty-state">검색 결과가 없습니다.</div>`;
        recordTable.innerHTML = `<div class="empty-state">환자 기록이 없습니다.</div>`;
        return;
    }

    const names = patientGroups.map(([name]) => name); // 환자별 이름 목록
    if (!state.selectedPatientName || !names.includes(state.selectedPatientName)) {
        state.selectedPatientName = names[0];
    }

    patientGroups.forEach(([name, records]) => { // 환자 이름 목록 버튼 생성
        const button = document.createElement("button");
        button.type = "button";
        button.className = `patient-list-button${name === state.selectedPatientName ? " active" : ""}`;
        button.innerHTML = `
            <span>${escapeHtml(name)}</span>
            <strong>${records.length}건</strong>
        `;
        button.addEventListener("click", () => {
            state.selectedPatientName = name;
            renderRecords();
        });
        patientList.appendChild(button);
    });

    const selectedRecords = patientGroups.find(([name]) => name === state.selectedPatientName)?.[1] || [];
    recordCardHeader.textContent = `${state.selectedPatientName} 환자 진료 기록`;

    selectedRecords.forEach((record, index) => { // 선택 환자의 진료 기록 카드를 최근순으로 출력
        const card = document.createElement("article");
        card.className = "record-card";
        card.innerHTML = `
            <div class="record-card-top">
                <strong>${index === 0 ? "최근 기록" : `${index + 1}번째 기록`}</strong>
                <span class="priority-badge ${priorityClass(record.priority)}">${record.priority}단계</span>
            </div>
            <dl>
                <div>
                    <dt>진료 시간</dt>
                    <dd>${escapeHtml(record.time)}</dd>
                </div>
                <div>
                    <dt>증상</dt>
                    <dd>${escapeHtml(record.symptom)}</dd>
                </div>
            </dl>
        `;
        recordTable.appendChild(card);
    });
}

// API 호출 함수
async function loadRecords() { // 서버에서 진료 기록 가져오는 함수
    const response = await fetch("/api/records");
    if (!response.ok) {
        throw new Error("records request failed");
    }
    const data = await response.json();
    state.records = data.records || [];
}

async function loadQueue() { // 서버에서 대기열을 가져오는 함수
    const response = await fetch("/api/queue");
    if (!response.ok) {
        throw new Error("queue request failed");
    }
    const data = await response.json();
    state.queue = data.queue || [];
}

async function refreshAll() { // 두 API 호출한 후 화면 전체 출력(새로 고침 버튼도 이 함수)
    try {
        await Promise.all([loadRecords(), loadQueue()]);
        renderSummary();
        renderQueue();
        renderRecords();
        setStatus("서버 연결됨", "ok");
    } catch (error) {
        setStatus("서버 연결 실패", "error");
    }
}

// 결과창
function showRegisterResult(data) {
    resultBox.hidden = false;
    resultBox.innerHTML = `
        <strong>${escapeHtml(data.name)}</strong> 환자가 ${escapeHtml(data.patientType)}로 접수되었습니다.<br>
        대기번호 ${data.patientNo}, 현재 증상 중요도 ${data.currentPriority}단계, 최종 중요도 ${data.finalPriority}단계<br>
        최근 기록: ${escapeHtml(data.recentRecord || "없음")}
    `;
} // 접수 성공 후 결과 박스

function showCompleteResult(data) { // 진료 완료 처리 결과를 결과 박스에 출력
    completeBox.hidden = false;

    if (!data.completed) {
        completeBox.textContent = "진료 완료 처리할 대기 환자가 없습니다.";
        return;
    }

    completeBox.innerHTML = `
        <strong>${escapeHtml(data.name)}</strong> 환자의 진료를 완료했습니다.
        (${escapeHtml(data.symptom)}, 중요도 ${data.finalPriority}단계)
    `;
}

// 행동과 기능을 연결 (이벤트 함수)
document.querySelectorAll(".nav-button").forEach((button) => { // 메뉴 버튼 연결
    button.addEventListener("click", () => switchView(button.dataset.view));
});

form.addEventListener("submit", async (event) => { // 환자 접수 폼 제출
    event.preventDefault();

    const selectedSymptoms = [...form.querySelectorAll('input[name="symptoms"]:checked')]
        .map((input) => input.value); // 체크된 증상만 모아서 서버로 전송

    if (selectedSymptoms.length === 0) {
        resultBox.hidden = false;
        resultBox.textContent = "증상을 하나 이상 선택해 주세요.";
        return;
    }

    // CHANGED: 여러 증상은 백엔드가 나누어 계산할 수 있도록 | 구분자로 전송
    const formData = new URLSearchParams();
    formData.set("name", document.querySelector("#nameInput").value.trim());
    formData.set("symptoms", selectedSymptoms.join("|"));

    const response = await fetch("/api/register", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: formData
    });

    if (!response.ok) {
        resultBox.hidden = false;
        resultBox.textContent = "접수 저장에 실패했습니다.";
        return;
    }

    const data = await response.json();
    showRegisterResult(data);
    form.reset();
    await refreshAll();
    switchView("queueView");
});

completeButton.addEventListener("click", async () => { // 진료 완료 버튼
    const response = await fetch("/api/complete", {
        method: "POST"
    });

    if (!response.ok) {
        completeBox.hidden = false;
        completeBox.textContent = "진료 완료 처리에 실패했습니다.";
        return;
    }

    const data = await response.json();
    showCompleteResult(data);
    await refreshAll();
});

// 검색 창 >> 검색어가 바뀔 때마다 기록 화면 다시 출력
searchInput.addEventListener("input", renderRecords);

// 새로 고침 버튼. refreshAll 사용
refreshButton.addEventListener("click", refreshAll);

// 페이지가 처음 열릴 때 서버에서 기록 가져와서 화면 출력
refreshAll();
